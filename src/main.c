#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>

#define BUF_SIZE 1024
#define MAX_EVENTS 10
#define MAX_FDS 65535

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1) {
        perror("socket");
        return 1;
    }

    if (set_nonblocking(listen_sock) == -1) {
        perror("fcntl");
        return 1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr = { INADDR_ANY },
    };

    if (bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(listen_sock, 10) == -1) {
        perror("listen");
        return 1;
    }

    // Create the epoll instance
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
        return 1;
    }

    // Add the listen socket
    struct epoll_event ev = {
        .events = EPOLLIN,
        .data.fd = listen_sock,
    };
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("epoll_ctl (listen_sock)");
        return 1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    struct sockaddr_in addrs[MAX_FDS];
    struct epoll_event events[MAX_EVENTS];

    for (;;) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            return 1;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == listen_sock) {
                client_addr_len = sizeof(client_addr);
                int fd = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);

                if (fd == -1) {
                    perror("accept");
                    return 1;
                }
                if (set_nonblocking(fd) == -1) {
                    perror("fcntl");
                    return 1;
                }
                // Add client socket to epoll instance
                ev.data.fd = fd;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
                    perror("epoll_ctl (client_sock)");
                    return 1;
                }

                // Add the client info to the client list
                printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                addrs[fd] = client_addr;
            } else {
                // Handle client
                char buf[BUF_SIZE];
                ssize_t bytes;
                int fd = events[i].data.fd;
                while ((bytes = read(fd, buf, sizeof(buf) - 1)) > 0) {
                    buf[bytes] = '\0';
                    printf("Message from %s:%d: %s", inet_ntoa(addrs[fd].sin_addr), ntohs(addrs[fd].sin_port), buf);

                    if (write(fd, buf, bytes) == -1) {
                        perror("write");
                        return 1;
                    }
                }
                if (bytes == 0) {
                    printf("Client disconnected: %s:%d\n", inet_ntoa(addrs[fd].sin_addr), ntohs(addrs[fd].sin_port));
                    close (fd);
                } else if (bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("read");
                    return 1;
                }
            }
        }
    }

    return 0;
}