// transfer — command-line test driver for FileTransferCore (macOS/Linux).
// Extra/unrecognized args are ignored on purpose.
// Usage:
//   transfer [serve] [-p port] [-d save_dir]           listen and receive pushes,
//                                                      serve pull/del requests
//   transfer send <ip> <port> <file> [cancel_ms]       connect + push a file;
//                                                      cancel_ms>0 cancels mid-send,
//                                                      then deletes the partial on the peer
//   transfer pull [-d save_dir] <ip> <port> <name>     pull a file from the peer
//   transfer del <ip> <port> <name>                    delete a cancelled partial on the peer

#include "TransferBridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <thread>

static volatile sig_atomic_t g_stop = 0;

// Async-signal-safe: only set the flag, cleanup runs on the main thread.
static void on_signal(int) { g_stop = 1; }

// Echo key events only; fine-grained progress is logged by the core at 1%.
static void on_progress(void*, uint64_t current, uint64_t total,
    const char* status, uint8_t event)
{
    if (event == FT_EVT_NONE) return;
    printf("[event %u] %s (%llu/%llu bytes)\n", event, status,
        (unsigned long long)current, (unsigned long long)total);
    fflush(stdout);
}

static FT_Handle create_engine(const char* saveDir)
{
    if (saveDir && mkdir(saveDir, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "cannot create save dir %s\n", saveDir);
        return NULL;
    }
    FT_Handle h = ft_create();
    if (!h) return NULL;
    if (saveDir) ft_set_save_path(h, saveDir);
    ft_set_progress_callback(h, on_progress, NULL);
    return h;
}

static const char* file_base_name(const char* path)
{
    const char* slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static int cmd_server(int argc, char** argv, int i)
{
    uint16_t port = 8800;
    const char* saveDir = "received";

    for (; i < argc; ++i) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = (uint16_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            saveDir = argv[++i];
        }
    }

    FT_Handle h = create_engine(saveDir);
    if (!h) return 1;

    if (ft_start_server(h, port) < 0) {
        fprintf(stderr, "start server on port %u failed\n", port);
        ft_destroy(h);
        return 1;
    }
    printf("Transfer server listening on port %u, saving to %s. Ctrl+C to stop.\n",
        port, saveDir);

    while (!g_stop) {
        sleep(1);
    }

    printf("shutting down...\n");
    ft_stop_server(h);
    ft_destroy(h);
    printf("bye\n");
    return 0;
}

static int cmd_send(int argc, char** argv, int i)
{
    const char* positional[3] = { NULL, NULL, NULL };
    int cancelMs = 0;
    int npos = 0;

    // No -d option here: a sender never writes files. Other args ignored.
    for (; i < argc; ++i) {
        if (npos < 3) {
            positional[npos++] = argv[i];
        } else if (npos == 3) {
            cancelMs = atoi(argv[i]);
            ++npos;
        }
    }
    if (npos < 3) {
        fprintf(stderr, "Usage: %s send <ip> <port> <file> [cancel_ms]\n", argv[0]);
        return 1;
    }

    FT_Handle h = create_engine(NULL);
    if (!h) return 1;
    if (ft_connect(h, positional[0], (uint16_t)atoi(positional[1])) < 0) {
        fprintf(stderr, "connect to %s:%s failed\n", positional[0], positional[1]);
        ft_destroy(h);
        return 1;
    }

    if (cancelMs > 0) {
        int sendRet = 0;
        std::thread sender([&] { sendRet = ft_send_file(h, positional[2]); });
        std::this_thread::sleep_for(std::chrono::milliseconds(cancelMs));
        printf(">>> cancelling\n");
        fflush(stdout);
        ft_cancel_transfer(h);
        sender.join();
        if (sendRet == -2) {
            printf(">>> asking peer to delete the partial file\n");
            fflush(stdout);
            ft_delete_file(h, file_base_name(positional[2]));
        }
        printf("send ret=%d\n", sendRet);
    } else {
        printf("send ret=%d\n", ft_send_file(h, positional[2]));
    }

    ft_disconnect(h);
    ft_destroy(h);
    return 0;
}

static int cmd_pull(int argc, char** argv, int i)
{
    const char* saveDir = "received";
    const char* positional[3] = { NULL, NULL, NULL };
    int npos = 0;

    for (; i < argc; ++i) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            saveDir = argv[++i];
        } else if (npos < 3) {
            positional[npos++] = argv[i];
        }
    }
    if (npos < 3) {
        fprintf(stderr, "Usage: %s pull [-d save_dir] <ip> <port> <name>\n", argv[0]);
        return 1;
    }

    FT_Handle h = create_engine(saveDir);
    if (!h) return 1;

    int ret = ft_request_file(h, positional[0], (uint16_t)atoi(positional[1]), positional[2]);
    printf("pull ret=%d\n", ret);

    ft_disconnect(h);
    ft_destroy(h);
    return 0;
}

static int cmd_delete(int argc, char** argv, int i)
{
    if (argc - i < 3) {
        fprintf(stderr, "Usage: %s del <ip> <port> <name>\n", argv[0]);
        return 1;
    }

    FT_Handle h = create_engine(NULL);
    if (!h) return 1;
    if (ft_connect(h, argv[i], (uint16_t)atoi(argv[i + 1])) < 0) {
        fprintf(stderr, "connect to %s:%s failed\n", argv[i], argv[i + 1]);
        ft_destroy(h);
        return 1;
    }

    int ret = ft_delete_file(h, argv[i + 2]);
    printf("delete ret=%d\n", ret);

    ft_disconnect(h);
    ft_destroy(h);
    return 0;
}

int main(int argc, char** argv)
{
    // Linux has no SO_NOSIGPIPE: ignore SIGPIPE so a closed peer yields EPIPE instead of killing the process.
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    const char* cmd = "serve";
    int i = 1;
    if (argc > 1 && (strcmp(argv[1], "serve") == 0 || strcmp(argv[1], "send") == 0 ||
        strcmp(argv[1], "pull") == 0 || strcmp(argv[1], "del") == 0)) {
        cmd = argv[1];
        i = 2;
    }

    if (strcmp(cmd, "serve") == 0) return cmd_server(argc, argv, i);
    if (strcmp(cmd, "send") == 0)   return cmd_send(argc, argv, i);
    if (strcmp(cmd, "pull") == 0)   return cmd_pull(argc, argv, i);
    return cmd_delete(argc, argv, i);
}
