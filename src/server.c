// 软件2201 苏一行 20229639
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include "game_struct.h"
#include "game_server.h"

#define PORT 8080
#define MAX_CLIENT MAX_PLAYERS
// #define _POSIX_C_SOURCE 200809L

Message message_test;
Player player_test;
Enemy enemies[MAX_ENEMIES];

// 记录当前连接的客户端数量
int active_clients = 0;

pthread_mutex_t client_count_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int client_sock;
    int index;
} ClientData;


void add_client_count(int n){
    // TODO 线程锁
    pthread_mutex_lock(&client_count_lock);
    active_clients += n;
    printf("active_client:%d\n", active_clients);
    pthread_mutex_unlock(&client_count_lock);
}

void update_player(int index, char ch){     // 接收客户端操作更新游戏数据
            switch (ch) {
            case 'w': // 上
                if (players[index].y > 0) players[index].y--;
                break;
            case 's': // 下
                if (players[index].y < max_y - 1) players[index].y++;
                break;
            case 'a': // 左
                if (players[index].x > 0) players[index].x--;
                break;
            case 'd': // 右
                if (players[index].x < max_x - 1) players[index].x++;
                break;
            case ' ': // 发射子弹
                fire_bullet(&players[index]);
                break;
        }
}

void* get_data_to_client(void* arg) {
    ClientData* data = (ClientData*) arg;
    int client_sock = data->client_sock;
    int index = data -> index;
    // free(data);
    char ch;
    while (1) {
        // 接收客户端发送的字符
        int recv_size = recv(client_sock, &ch, sizeof(ch), 0);
        if (recv_size <= 0) {
            if (recv_size == 0) {
                printf("Client disconnected\n");
            } else {
                perror("Recv failed");
            }
            break;
        }
        // 打印接收到的字符
        printf("Received character from client %d: %c\n",index, ch);
        update_player(index, ch);
    }
    close(client_sock);
    return NULL;
}

void initData(int index){
    init_player(&players[index], 10, index + 1, 3);
    init_enemies(enemies);
    message = toMessage(players, enemies, active_clients, index);
}

int updata_state(int game_over, int index, int client_sock){
    // 生成敌机
    if (rand() % 20 == 0) {
        spawn_enemy(enemies);
    }

    // 更新状态
    for (int i = 0; i < MAX_PLAYERS; i++) {  // 更新飞机状态
        update_bullets(&players[i]);
        check_collision(&players[i], enemies);
    }
    update_enemies(enemies);    // 更新敌机状态

    game_over |= check_player_collision(&players[index], enemies, client_sock);

    return game_over;
}

void find_all_client_exit(){
    pthread_mutex_lock(&client_count_lock);
    if (active_clients == 0) {
        pthread_mutex_unlock(&client_count_lock);
        printf("所有客户端均已结束运行\n");
        exit(0);
    }
    pthread_mutex_unlock(&client_count_lock);
    pthread_exit(NULL);
}

// TODO 多线程
void* handle_client_to_AllData(void* arg){

    ClientData* data = (ClientData*) arg;
    int client_sock = data->client_sock;
    Message message_server;
    int index = data -> index;

    initData(index);

    int game_over = 0;

    while(!game_over) {

        // 更新游戏数据
        game_over = updata_state(game_over, index, client_sock);

        if (game_over == 1){        // 游戏结束，发送信号终止客户端
            message_server.index = index;
            printf("Player %d exit\n", index);
            
            // 发送 Ctrl+C 信号给客户端
            char ctrl_c = SIGINT_CHAR;
            send(client_sock, &ctrl_c, sizeof(ctrl_c), 0);
            
            add_client_count(-1);
            // 检查是否所有客户端都已退出
            find_all_client_exit();
            break;
        } else {        // 游戏正常进行，发送封装好的数据包
            message_server = message;
        }

        // TODO 序列化处理
        char buffer[1024];  // 用于存储序列化数据
        serialize_message(&message_server, buffer);

        if (send(client_sock, &buffer, sizeof(buffer), 0) < 0) {
            perror("Send message faild");
            close(client_sock);
            // 减少活动客户端计数
            add_client_count(-1);

            // 检查是否所有客户端都已退出
            find_all_client_exit();
            // break;
        }
        // 更新结构体数据
        // updataAllData();

        struct timespec ts;
        ts.tv_sec = 0;  // 秒
        ts.tv_nsec = DELAY * active_clients;  // 纳秒
        nanosleep(&ts, NULL);  
    }
    close(client_sock);
    return NULL;
}

int main(){

    // 获取终端大小
    get_terminal_xy();

    printf("x:%d y:%d\n", max_x, max_y);

    signal(SIGPIPE, SIG_IGN);
    int server_fd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t sent_thread_id, get_thread_id;
    int client_count = 0;

    // 创建服务器套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr. sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定套接字到端口
    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))){
        perror("Bind failed");
        return -1;        
    }

    // 监听端口
    if (listen(server_fd, MAX_CLIENT) == -1){
        perror("cannot failed");
        return -1;
    }

    printf("server listening on port %d ... \n", PORT);
    int flag = 0;
    while(1) {

        // 接收客户端连接
        if((client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len)) == -1){
            perror("Accept failed");
            continue;
        }

        printf("Connection established with client\n");

        // 增加活动客户端计数
        add_client_count(1);

        // 创建一个结构体数组，以便传递给线程
        ClientData* data = (ClientData*)malloc(sizeof(ClientData));
        data->client_sock = client_sock;
        data->index = client_count % MAX_CLIENT;
        client_count++;
        
        // 创建新的线程发送数据
        if (pthread_create(&sent_thread_id, NULL, handle_client_to_AllData, (void*)data) != 0){
            perror("sent_thread creation failed");
            close(client_sock);
            free(data);
        }

        // 创建新的线程接收不同客户端的输入结果
        if (pthread_create(&get_thread_id, NULL, get_data_to_client, (void*)data) != 0){
            perror("get_thread creation failed");
            close(client_sock);
            free(data);
        }
        sleep(1);
        free(data);
        //分离线程，使得线程结束后自动清理
        pthread_detach(sent_thread_id);
        pthread_detach(get_thread_id);
    }

    // 关闭服务器套接字
    close(server_fd);
    return 0;

}
