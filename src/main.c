#include "connection.h"
#include "server.h"
#include "event_loop.h"
#include "config.h"
#include "log.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    server_conf_init(&sv_conf);
    parse_args(argc, argv, &sv_conf);

    struct sigaction sa = {
        .sa_handler = sig_handler,
    };
    if (sigaction(SIGTERM, &sa, NULL) == -1 || sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(sv_conf.port),
        .sin_addr = { INADDR_ANY },
    };
    int listen_sock = start_server(&server_addr);
    if (listen_sock == -1) {
        perror("start_server");
        return EXIT_FAILURE;
    }

    LOG_INFO("listening on port %d, root: %s", sv_conf.port, sv_conf.root_dir);

    struct connection server_conn = {
        .fd = listen_sock,
        .addr = server_addr,
    };

    struct epoll_instance epoll_inst;
    if (init_epoll_instance(&epoll_inst, &server_conn) == -1) {
        perror("init_epoll_instance");
        return EXIT_FAILURE;
    }

    if (start_event_loop(&epoll_inst) == -1) {
        perror("start_event_loop");
        return EXIT_FAILURE;
    }

    if (close(epoll_inst.epfd) == -1 || close(listen_sock) == -1) {
        if (errno != EBADF) {
            perror("close");
            return EXIT_FAILURE;
        }
    }
    LOG_INFO("server exiting");

    return EXIT_SUCCESS;
}