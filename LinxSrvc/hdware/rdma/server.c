#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>
#include <arpa/inet.h>

#define DEFAULT_PORT "12345"
#define DEFAULT_BUFFER_SIZE 1024

struct context {
    struct ibv_mr* mr;
    char* buff;
    struct rdma_cm_id* id;
};

static volatile int keep_running = 1;

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
        exit(EXIT_FAILURE); \
    } \
} while (0)

/* Custom implementation of rdma_reg_msgs using ibv_reg_mr.
   It registers the given buffer (or NULL) for the specified length
   on the protection domain associated with the rdma_cm_id.
   Access flags include local write and remote read/write. */
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

void handle_cm_event(struct rdma_event_channel* ec)
{
    struct rdma_cm_event* event = NULL;
    struct context* ctx = NULL;

    while (keep_running) {
        if (rdma_get_cm_event(ec, &event)) {
            fprintf(stderr, "Failed to get CM event, exiting event loop\n");
            break;
        }
        switch (event->event) {
        case RDMA_CM_EVENT_CONNECT_REQUEST:
            printf("Client connection request\n");
            ctx = malloc(sizeof(struct context));
            CHECK_PTR(ctx, "Failed to allocate context");
            memset(ctx, 0, sizeof(*ctx));
            ctx->id = event->id;
            ctx->id->context = ctx;

            // Allocate and initialize memory
            ctx->buff = malloc(DEFAULT_BUFFER_SIZE);
            CHECK_PTR(ctx->buff, "Failed to allocate buffer");
            memset(ctx->buff, 0, DEFAULT_BUFFER_SIZE);

            ctx->mr = rdma_reg_msgs(ctx->id, ctx->buff, DEFAULT_BUFFER_SIZE);
            if (!ctx->mr) {
                fprintf(stderr, "Memory registration failed\n");
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
            break;

        case RDMA_CM_EVENT_ESTABLISHED:
            printf("Connection established\n");
            break;

        case RDMA_CM_EVENT_DISCONNECTED:
            printf("Connection disconnected\n");
            // ctx is the current connection context and can be freed
            ctx = event->id->context;
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
    char* port = (argc > 1) ? argv[1] : DEFAULT_PORT;
    struct rdma_event_channel* ec = NULL;
    struct rdma_cm_id* listen_id = NULL;
    struct sockaddr_in addr;

    // Install signal handler for interrupts
    signal(SIGINT, int_handler);

    // Create event channel
    ec = rdma_create_event_channel();
    CHECK_PTR(ec, "Failed to create event channel");
    CHECK_INT(rdma_create_id(ec, &listen_id, NULL, RDMA_PS_TCP), "Failed to create ID");

    // Bind address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atoi(port));
    CHECK_INT(rdma_bind_addr(listen_id, (struct sockaddr*)&addr), "Failed to bind address");
    CHECK_INT(rdma_listen(listen_id, 5), "Failed to listen");

    printf("Server listening on port %s...\n", port);

    // Enter event loop
    handle_cm_event(ec);

    // Clean up before exiting
    rdma_destroy_id(listen_id);
    rdma_destroy_event_channel(ec);

    printf("Server terminated.\n");
    return 0;
}
