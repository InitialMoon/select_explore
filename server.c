/**
 * copyright: https://blog.csdn.net/zujipi8736/article/details/86606093 & gpt
 */
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

/**
 *  select 的典型编程模型
 * 
 * 1.创建和初始化文件描述符集合：使用 FD_ZERO 初始化集合，并通过 FD_SET 添加需要监控的文件描述符。
 * 2.调用 select 进行监听： 根据指定的文件描述符集合和超时时间，阻塞等待事件。
 * 3.检查就绪的文件描述符：通过 FD_ISSET 检查哪些文件描述符已就绪，进行相应的处理。
 * 4.循环监听：监听是一个循环过程，需要持续重新设置文件描述符集合，调用 select。
 */

/**
 * fd_set优势
 *  对比总结
 *  •	位图（fd_set）：
 *      •	占用 128 字节，可表示 1024 个文件描述符的状态（每个文件描述符用 1 位）。
 *      •	每个文件描述符占用 1 位（0 或 1）。
 *  •	数组（int）：
 *      •	如果用一个 int 表示一个文件描述符，则 128 字节 的内存最多可表示 32 个文件描述符。
 *      •	每个文件描述符占用 4 字节。
 */
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
    FD_ZERO(&all_fds); // 初始化：清空文件描述符集合（fd_set 类型）。
    FD_SET(server_fd, &all_fds); // 将服务器监听的文件描述符 server_fd 添加到文件描述符集合 all_fds 中。
    max_fd = server_fd; // 初始化 max_fd 为当前的最大文件描述符值，即 server_fd

    printf("Server started on port %d\n", PORT);

    struct timeval tv; // 可以设置超时时间，控制 select 是否阻塞，以及阻塞的时间长度。
    tv.tv_sec = 5; // 秒
    tv.tv_usec = 0; // 微秒

    while (1) {
        read_fds = all_fds;

        // void FD_CLR(int fd, fd_set *set);//清除某一个被监视的文件描述符。
        // int  FD_ISSET(int fd, fd_set *set);//测试一个文件描述符是否是集合中的一员
        // void FD_SET(int fd, fd_set *set);//添加一个文件描述符，将set中的某一位设置成1
        // void FD_ZERO(fd_set *set);//清空集合中的文件描述符,将每一位都设置为0

        // 使用select监听多个socket
        // 函数用于监视文件描述符的变化情况。当文件描述符状态发生变化时，select() 会返回，此时可以通过 FD_ISSET() 来检查文件描述符的状态。
        // 为了维护fd_set类型的参数，会使用下面四个宏：FD_SET(), FD_CLR(),
        // FD_ZERO() 和 FD_ISSET()。
        // @param nfds	sets的文件描述符的最大值
        // @param readfds	fd_set 类型，包含了需要检查是否可读的描述符，输出时表示哪些描述符可读。可为 NULL。
        // @param writefds	fd_set 类型，包含了需要检查是否可写的描述符，输出时表示哪些描述符可写。可为 NULL。
        // @param errorfds	fd_set 类型，包含了需要检查是否出错的描述符，输出时表示哪些描述符出错。可为 NULL。
        // @param timeout   struct timeval 类型的结构体，表示等待检查完成的最长时间。
        // @return	成功时返回可读、可写或出错的文件描述符的数量，超时返回 0，出错返回 -1。
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if (activity < 0) {
            perror("select error");
            exit(-1);
        }

        if (activity == 0) {
            printf("Timeout: No activity detected.\n");
            continue;
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
                    continue;
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