/* cspell:ignore rdma rdma_cma infiniband ibverbs rkey lkey dereg qp cq conn_id ec */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define DEFAULT_SERVER_IP "127.0.0.1"  // default server IP; change if needed
#define DEFAULT_PORT "12345"
#define DEFAULT_BUFFER_SIZE 1024

// Macro for error checking
#define CHECK(condition, msg) do { \
    if (condition) { \
        perror(msg); \
        exit(EXIT_FAILURE); \
    } \
} while(0)

struct context {
    struct ibv_mr* local_mr;
    struct ibv_mr* remote_mr;
    char* local_buffer;
    char* remote_buffer;
    struct rdma_cm_id* id;
};

void handle_cm_event(struct rdma_event_channel* ec)
{
    struct rdma_cm_event* event;
    struct context* ctx = NULL;

    CHECK(rdma_get_cm_event(ec, &event), "Failed to get CM event");
    if (event->event == RDMA_CM_EVENT_ESTABLISHED) {
        ctx = (struct context*)event->id->context;

        // Retrieve remote memory information
        uint32_t remote_rkey = *((uint32_t*)event->param.conn.private_data);
        // Simulated registration of remote memory using our custom rdma_reg_msgs
        ctx->remote_mr = ibv_reg_mr(ctx->id->pd, NULL, DEFAULT_BUFFER_SIZE, IBV_ACCESS_REMOTE_WRITE);
        CHECK(!ctx->remote_mr, "Remote memory registration failed");
        ctx->remote_mr->rkey = remote_rkey;
        ctx->remote_buffer = (char*)ctx->remote_mr->addr;

        // Execute RDMA write
        struct ibv_send_wr wr, *bad_wr;
        struct ibv_sge sge = {
            .addr = (uintptr_t)ctx->local_buffer,
            .length = DEFAULT_BUFFER_SIZE,
            .lkey = ctx->local_mr->lkey
        };
        wr = (struct ibv_send_wr){
            .wr_id = 1,
            .sg_list = &sge,
            .num_sge = 1,
            .opcode = IBV_WR_RDMA_WRITE,
            .send_flags = IBV_SEND_SIGNALED,
            .wr.rdma = {
                .remote_addr = (uintptr_t)ctx->remote_buffer,
                .rkey = ctx->remote_mr->rkey
            }
        };
        CHECK(ibv_post_send(ctx->id->qp, &wr, &bad_wr), "Posting send request failed");

        // Wait for the completion event
        struct ibv_wc wc;
        while (ibv_poll_cq(ctx->id->send_cq, 1, &wc) == 0);
        if (wc.status == IBV_WC_SUCCESS)
            printf("Data written successfully!\n");
        else
            fprintf(stderr, "Error: %s\n", ibv_wc_status_str(wc.status));

        rdma_disconnect(ctx->id);
    }
    rdma_ack_cm_event(event);
}

int main(int argc, char* argv[])
{
    char* ip = DEFAULT_SERVER_IP;
    char* port = DEFAULT_PORT;
    if (argc >= 2) {
        ip = argv[1];
    }
    if (argc >= 3) {
        port = argv[2];
    }

    struct rdma_event_channel* ec;
    struct rdma_cm_id* conn_id;
    struct sockaddr_in addr;
    struct context ctx = { 0 };

    // Initialize RDMA channel
    ec = rdma_create_event_channel();
    CHECK(!ec, "Creating event channel failed");
    CHECK(rdma_create_id(ec, &conn_id, NULL, RDMA_PS_TCP), "Creating CM ID failed");
    ctx.id = conn_id;
    conn_id->context = &ctx;

    // Parse server address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    CHECK(inet_pton(AF_INET, ip, &addr.sin_addr) != 1, "inet_pton failed");

    // Register local memory using our custom rdma_reg_msgs
    ctx.local_buffer = malloc(DEFAULT_BUFFER_SIZE);
    memset(ctx.local_buffer, 'A', DEFAULT_BUFFER_SIZE);  // fill with test data
    ctx.local_mr = ibv_reg_mr(conn_id->pd, ctx.local_buffer, DEFAULT_BUFFER_SIZE, IBV_ACCESS_LOCAL_WRITE);
    CHECK(!ctx.local_mr, "Local memory registration failed");

    // Initiate connection
    struct rdma_conn_param param = { 0 };
    CHECK(rdma_connect(conn_id, &param), "Connection failed");
    CHECK(rdma_resolve_addr(conn_id, NULL, (struct sockaddr*)&addr, 2000), "Address resolution failed");

    printf("Client connected\n");
    handle_cm_event(ec);  // Process events

    // Clean up resources
    ibv_dereg_mr(ctx.local_mr);
    ibv_dereg_mr(ctx.remote_mr);
    free(ctx.local_buffer);
    rdma_destroy_id(conn_id);
    rdma_destroy_event_channel(ec);

    return 0;
}
