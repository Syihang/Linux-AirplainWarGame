#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h> // 用于修改终端设置
#include "game_struct.h"
#include "game_client.h"

#define PORT 8080
#define SERVER_ADDR "127.0.0.1"
#define SIGINT_CHAR '1'  // Ctrl+C 的 ASCII 码   

Message message_client;

int sock; // 将套接字声明为全局变量，以便线程可以访问

// 关闭回显和行缓冲模式
void set_noncanonical_mode() {
    struct termios oldt, newt;

    // 获取当前终端设置
    tcgetattr(STDIN_FILENO, &oldt);

    // 复制当前设置以便恢复
    newt = oldt;

    // 禁用回显（ECHO）和行缓冲模式（ICANON）
    newt.c_lflag &= ~(ICANON | ECHO);

    // 设置新的终端设置
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

// 恢复终端默认模式
void restore_terminal_mode() {
    struct termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    oldt.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    clear();
    refresh();
    endwin();      
}

// 键盘输入线程函数
void* keyboard_input_thread(void* arg) {
    char input;
    while (1) {
        // 从键盘读取单个字符输入（不需要按回车）
        input = getchar();
        if (input == 'q') { // 如果输入 'q'，退出程序
            printf("Exiting...\n");
            close(sock);
            restore_terminal_mode(); // 恢复终端默认模式
            exit(0);
        }

        // 将输入发送给服务器
        ssize_t bytes_sent = send(sock, &input, sizeof(input), 0);
        if (bytes_sent <= 0) {
            perror("Send failed");
            break;
        }
        // printf("Sent: %c\n", input);
    }
    return NULL;
}

void print_Message(){
        // 打印接收到的结构体数据
        // printf("AllData test: %d\n", alldata.index);
        printf("%d\n", message_client.player_number);
        printf("coler:%d\n", message_client.players->color_pair);
        printf("x:%d y:%d\n", message_client.players->x, message_client.players->y);
        printf("index:%d\n", message_client.index);
        printf("\n");
}

int init_sock(){

    struct sockaddr_in server_addr;

    // 创建服务器套接字
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // 将服务器IP地址转换为二进制
    if (inet_pton(AF_INET, SERVER_ADDR, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }

    // 连接到服务器
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connect failed");
        return -1;
    }

    printf("Connected to server at %s:%d\n", SERVER_ADDR, PORT);
}

int get_message(){
    // 先检查是否收到控制信号
    char signal_check;
    ssize_t signal_bytes = recv(sock, &signal_check, 1, MSG_PEEK);
    if (signal_bytes > 0 && signal_check == SIGINT_CHAR) {
        recv(sock, &signal_check, 1, 0); // 实际读取该字符
        restore_terminal_mode();
        endwin();  // 清理 ncurses
        close(sock);
        // clear();
        draw_game_over(message_client.players[message_client.index].score);
        restore_terminal_mode();
        exit(0);
    }

    // 正常的消息处理
    char recv_buffer[1024];
    ssize_t bytes_received = recv(sock, &recv_buffer, sizeof(recv_buffer), 0);
    if (bytes_received <= 0) {
        perror("Recv failed");
        return -1;
    } else{
        deserialize_message(recv_buffer, &message_client);
        return 0;
    }
}

int main() {

    // 初始化ncurses的设置
    init_ncurses();

    // 设置终端为非规范模式（关闭回显和行缓冲）
    set_noncanonical_mode();

    // 构造sock
    init_sock();
            
    init_colors(message_client);

    // 创建键盘输入线程
    pthread_t input_thread;
    if (pthread_create(&input_thread, NULL, keyboard_input_thread, NULL) != 0) {
        perror("Failed to create keyboard input thread");
        restore_terminal_mode(); // 恢复终端默认模式
        return -1;
    }

    // 主线程接收数据
    int game_over = 0;
    while (!game_over) {

        // 获取message数据
        if(get_message() == -1){
            break;
        }
        // 渲染游戏元素
        showGame(message_client);

    }

    // showGameDown(message_client.players[message_client.index].score);

    // 等待线程结束
    pthread_join(input_thread, NULL);

    // 恢复终端默认模式
    restore_terminal_mode();

    close(sock);

    return 0;
}