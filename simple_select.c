#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

int main() {
    fd_set read_fds;
    FD_ZERO(&read_fds);          // 初始化集合
    FD_SET(STDIN_FILENO, &read_fds); // 将标准输入加入集合

    printf("Waiting for input...\n");

    struct timeval timeout;
    timeout.tv_sec = 2;          // 超时时间：5秒
    timeout.tv_usec = 0;

    int ret = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);
    if (ret == -1) {
        perror("select failed");
    } else if (ret == 0) {
        printf("Timeout occurred, no input.\n");
    } else {
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            printf("Data is available to read!\n");
        }
    }

    return 0;
}