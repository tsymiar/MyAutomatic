#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>
#include <arpa/inet.h>

#define RDMA_SERVER_PORT "12345"
#define SERVER_BUFFER_SIZE 1024
#define SERVER_QUEUE_DEPTH 5

struct context {
    struct ibv_mr* mr;
    char* buff;
    struct rdma_cm_id* id;
    struct context* next; // For managing multiple connections
};

static volatile int keep_running = 1;
static struct context* ctx_list = NULL; // Head of the connection context list
static pthread_mutex_t ctx_list_lock = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread safety

void int_handler(int dummy)
{
    keep_running = 0;
}

// Macros to check the return value for pointer or integer functions
#define CHECK_INT(fn, msg) do { \
    if ((fn) != 0) { \
        fprintf(stderr, "Error: %s (%s:%d) - %s\n", msg, __FILE__, __LINE__, strerror(errno)); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#define CHECK_PTR(ptr, msg) do { \
    if (!(ptr)) { \
        fprintf(stderr, "Error: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        perror("Details"); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

/* Custom implementation of rdma_reg_msgs using ibv_reg_mr. */
struct ibv_mr* rdma_reg_msgs(struct rdma_cm_id* id, void* buf, size_t length)
{
    return ibv_reg_mr(id->pd, buf, length,
        IBV_ACCESS_LOCAL_WRITE |
        IBV_ACCESS_REMOTE_WRITE |
        IBV_ACCESS_REMOTE_READ);
}

// Helper function to free resources
void cleanup_context(struct context* ctx)
{
    if (ctx) {
        if (ctx->mr)
            ibv_dereg_mr(ctx->mr);
        if (ctx->buff)
            free(ctx->buff);
        if (ctx->id)
            rdma_destroy_qp(ctx->id);
        free(ctx);
    }
}

void add_context(struct context* ctx)
{
    pthread_mutex_lock(&ctx_list_lock);
    ctx->next = ctx_list;
    ctx_list = ctx;
    pthread_mutex_unlock(&ctx_list_lock);
}

void remove_context(struct context* ctx)
{
    pthread_mutex_lock(&ctx_list_lock);
    struct context** current = &ctx_list;
    while (*current) {
        if (*current == ctx) {
            *current = ctx->next;
            break;
        }
        current = &(*current)->next;
    }
    pthread_mutex_unlock(&ctx_list_lock);
}

void handle_cm_event(struct rdma_event_channel* ec)
{
    struct rdma_cm_event* event = NULL;

    while (keep_running) {
        if (rdma_get_cm_event(ec, &event)) {
            fprintf(stderr, "fails to get CM event, exiting event loop\n");
            break;
        }

        struct context* ctx = NULL;
        switch (event->event) {
        case RDMA_CM_EVENT_CONNECT_REQUEST:
            printf("Client connection request\n");
            ctx = malloc(sizeof(struct context));
            CHECK_PTR(ctx, "fails to allocate context");
            memset(ctx, 0, sizeof(*ctx));
            ctx->id = event->id;
            ctx->id->context = ctx;

            // Allocate and initialize memory
            ctx->buff = malloc(SERVER_BUFFER_SIZE);
            CHECK_PTR(ctx->buff, "fails to allocate buffer");
            memset(ctx->buff, 0, SERVER_BUFFER_SIZE);

            ctx->mr = rdma_reg_msgs(ctx->id, ctx->buff, SERVER_BUFFER_SIZE);
            if (!ctx->mr) {
                fprintf(stderr, "Memory registration fails\n");
                cleanup_context(ctx);
                rdma_ack_cm_event(event);
                continue;
            }

            // Accept the connection and send the memory key as private data
            {
                struct rdma_conn_param param;
                memset(&param, 0, sizeof(param));
                param.responder_resources = 1;
                param.private_data = &ctx->mr->rkey;
                param.private_data_len = sizeof(ctx->mr->rkey);
                if (rdma_accept(ctx->id, &param)) {
                    fprintf(stderr, "Failed to accept connection\n");
                    cleanup_context(ctx);
                    break;
                }
            }

            add_context(ctx); // Add to the global context list
            break;

        case RDMA_CM_EVENT_ESTABLISHED:
            printf("Connection established\n");
            ctx = event->id->context;
            // Send a welcome message
            snprintf(ctx->buff, SERVER_BUFFER_SIZE, "Welcome to RDMA server!");
            // Post a send work request
            struct ibv_sge sge;
            struct ibv_send_wr wr, * bad_wr = NULL;

            memset(&sge, 0, sizeof(sge));
            sge.addr = (uintptr_t)ctx->buff;
            sge.length = strlen(ctx->buff) + 1;
            sge.lkey = ctx->mr->lkey;

            memset(&wr, 0, sizeof(wr));
            wr.wr_id = (uintptr_t)ctx;
            wr.sg_list = &sge;
            wr.num_sge = 1;
            wr.opcode = IBV_WR_SEND;
            wr.send_flags = IBV_SEND_SIGNALED;

            CHECK_INT(ibv_post_send(ctx->id->qp, &wr, &bad_wr), "fails to send message");
            break;

        case RDMA_CM_EVENT_DISCONNECTED:
            printf("Connection disconnected\n");
            ctx = event->id->context;
            remove_context(ctx); // Remove from the global context list
            cleanup_context(ctx);
            break;

        default:
            fprintf(stderr, "Unknown event: %d\n", event->event);
        }
        rdma_ack_cm_event(event);
    }
}

int main(int argc, char* argv[])
{
    char* port = (argc > 1) ? argv[1] : RDMA_SERVER_PORT;
    int queue_depth = (argc > 2) ? atoi(argv[2]) : SERVER_QUEUE_DEPTH;
    struct rdma_event_channel* ec = NULL;
    struct rdma_cm_id* listen_id = NULL;
    struct sockaddr_in addr;

    // Install signal handler for interrupts
    signal(SIGINT, int_handler);

    // Enumerate RDMA devices
    struct ibv_device** dev_list = ibv_get_device_list(NULL);
    CHECK_PTR(dev_list, "fails to get RDMA device list");

    printf("Available RDMA devices:\n");
    for (struct ibv_device** dev = dev_list; *dev != NULL; ++dev) {
        printf("  - %s\n", ibv_get_device_name(*dev));
    }
    // Free the device list after use
    ibv_free_device_list(dev_list);

    // Create event channel
    ec = rdma_create_event_channel();
    CHECK_PTR(ec, "fails to create event channel");
    CHECK_INT(rdma_create_id(ec, &listen_id, NULL, RDMA_PS_TCP), "fails to create ID");

    // Bind address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atoi(port));
    CHECK_INT(rdma_bind_addr(listen_id, (struct sockaddr*)&addr), "fails to bind address");
    CHECK_INT(rdma_listen(listen_id, queue_depth), "fails to listen");

    printf("Server listening on port %s with queue depth %d...\n", port, queue_depth);

    // Enter event loop
    handle_cm_event(ec);

    // Clean up before exiting
    while (ctx_list) {
        struct context* ctx = ctx_list;
        ctx_list = ctx_list->next;
        cleanup_context(ctx);
    }
    rdma_destroy_id(listen_id);
    rdma_destroy_event_channel(ec);

    printf("Server terminated.\n");
    return 0;
}
