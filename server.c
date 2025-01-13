#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 12345          // 服务器监听的端口号
#define MAX_CLIENTS 1024    // 最大客户端数量
#define BUFFER_SIZE 4       // 接收的报文大小（4字节）

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    fd_set read_fds, all_fds;
    int max_fd;

    int client_ids[MAX_CLIENTS];      // 存储每个客户端的id
    int client_counts[MAX_CLIENTS];  // 存储每个客户端接收的报文数量
    int client_sockets[MAX_CLIENTS]; // 存储每个客户端的socket
    int client_count = 0;            // 当前已连接的客户端数量

    // 初始化数据
    memset(client_ids, -1, sizeof(client_ids));
    memset(client_counts, 0, sizeof(client_counts));
    memset(client_sockets, -1, sizeof(client_sockets));

    // 创建服务器socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(-1);
    }

    // 设置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        close(server_fd);
        exit(-1);
    }

    // 监听
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen failed");
        close(server_fd);
        exit(-1);
    }

    // 初始化fd集合
    FD_ZERO(&all_fds); // 清空文件描述符集合（fd_set 类型）。
    FD_SET(server_fd, &all_fds); // 将服务器监听的文件描述符 server_fd 添加到文件描述符集合 all_fds 中。
    max_fd = server_fd; // 初始化 max_fd 为当前的最大文件描述符值，即 server_fd

    printf("Server started on port %d\n", PORT);

    while (1) {
        read_fds = all_fds;

        // 使用select监听多个socket
        // @details select() 函数用于监视文件描述符的变化情况。当文件描述符状态发生变化时，select() 会返回，此时可以通过 FD_ISSET() 来检查文件描述符的状态。
        // 为了维护fd_set类型的参数，会使用下面四个宏：FD_SET(), FD_CLR(), FD_ZERO() 和 FD_ISSET()。
        // @param nfds	sets的文件描述符的最大值
        // @param readfds	fd_set 类型，包含了需要检查是否可读的描述符，输出时表示哪些描述符可读。可为 NULL。
        // @param writefds	fd_set 类型，包含了需要检查是否可写的描述符，输出时表示哪些描述符可写。可为 NULL。
        // @param errorfds	fd_set 类型，包含了需要检查是否出错的描述符，输出时表示哪些描述符出错。可为 NULL。
        // @param timeout   struct timeval 类型的结构体，表示等待检查完成的最长时间。
        // @return	成功时返回可读、可写或出错的文件描述符的数量，超时返回 0，出错返回 -1。
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select error");
            exit(-1);
        }

        // 检查是否有新连接
        if (FD_ISSET(server_fd, &read_fds)) {
            client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (client_fd == -1) {
                perror("accept failed");
                exit(-1);
            }

            // 分配客户端ID
            if (client_count >= MAX_CLIENTS) {
                printf("Max clients reached. Rejecting new connection.\n");
                close(client_fd);
                continue;
            }

            client_ids[client_count] = client_count;
            client_sockets[client_count] = client_fd;
            client_counts[client_count] = 0;

            FD_SET(client_fd, &all_fds);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }

            printf("connect from %d\n", client_count);
            client_count++;
        }

        // 检查客户端消息
        for (int i = 0; i < client_count; i++) {
            int sock = client_sockets[i];
            if (sock != -1 && FD_ISSET(sock, &read_fds)) {
                char buffer[BUFFER_SIZE] = {0};
                int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);

                if (bytes_received <= 0) {
                    // 连接断开
                    printf("Client %d disconnected\n", i);
                    close(sock);
                    FD_CLR(sock, &all_fds);
                    client_sockets[i] = -1;
                    exit(-1);
                }

                // 更新客户端报文计数
                client_counts[i]++;
                printf("received from %d\n", i);

                // 打印所有客户端的报文数量
                for (int j = 0; j < client_count; j++) {
                    if (client_sockets[j] != -1) {
                        printf("%d: %d\n", j, client_counts[j]);
                    }
                }
                printf("------\n");
            }
        }
    }

    close(server_fd);
    return 0;
}