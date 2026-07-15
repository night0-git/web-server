#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>

#define BUF_SIZE 1024

int main() {
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr = {0},
    };

    if (bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(listen_sock, 10) == -1) {
        perror("listen");
        return 1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int client_sock;
    while (true) {
        client_addr_len = sizeof(client_sock);
        client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock == -1) {
            perror("accept");
            return 1;
        }
        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            // Child
            close(listen_sock);

            // Handle client
            char buf[BUF_SIZE];
            ssize_t bytes;
            while ((bytes = read(client_sock, buf, sizeof(buf))) > 0) {
                buf[bytes] = '\0';
                printf("Message from %s:%s: %s", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buf);

                if (write(client_sock, buf, bytes) == -1) {
                    perror("write");
                    return 1;
                }
            }

            close (client_sock);
            _exit(0);
        } else {
            // Parent
            close(client_sock);
        }
    }

    return 0;
}