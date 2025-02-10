#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include "game_struct.h"

Message message;              // 数据包
Player players[MAX_PLAYERS];  // 静态数组
#define SIGINT_CHAR '1'  // Ctrl+C 的 ASCII 码
// Enemy enemies[MAX_ENEMIES];   // 静态数组

void get_terminal_xy() {
    struct winsize w;
    // 获取终端大小
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        exit(EXIT_FAILURE);
    }
    max_y = w.ws_row;
    max_x = w.ws_col;
}

// 初始化玩家
void init_player(Player *player, int max_bullets, int color_pair, int bullet_color) {
    player->x = max_x / 2;
    player->y = max_y - 2;
    player->score = 0;
    player->color_pair = color_pair;
    player->bullet_color = bullet_color;
    player->max_bullets = max_bullets;
    player->bullets = (Bullet *)malloc(max_bullets * sizeof(Bullet));
    for (int i = 0; i < max_bullets; i++) {
        player->bullets[i].active = 0;
        // printf("%d\n", player->bullets[i].active);
    }
}

// 初始化敌机
void init_enemies(Enemy *enemies) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].active = 0;
    }
}

// 生成敌机
void spawn_enemy(Enemy *enemies) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            enemies[i].x = rand() % max_x;
            enemies[i].y = 0;
            enemies[i].active = 1;
            break;
        }
    }
}

// 更新敌机位置
void update_enemies(Enemy* enemies) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            enemies[i].y++;
            if (enemies[i].y >= max_y) {
                enemies[i].active = 0; // 超出屏幕范围，销毁
            }
        }
    }
}

// 发射子弹
void fire_bullet(Player *player) {
    for (int i = 0; i < player->max_bullets; i++) {
        if (!player->bullets[i].active) {
            player->bullets[i].x = player->x;
            player->bullets[i].y = player->y - 1;
            player->bullets[i].active = 1;
            break;
        }
    }
}

// 更新子弹位置
void update_bullets(Player *player) {
    for (int i = 0; i < player->max_bullets; i++) {
        if (player->bullets[i].active) {
            player->bullets[i].y--;
            if (player->bullets[i].y < 0) {
                player->bullets[i].active = 0; // 超出屏幕范围，销毁
            }
        }
    }
}

// 检测子弹是否击中敌机
void check_collision(Player *player, Enemy *enemies) {
    for (int i = 0; i < player->max_bullets; i++) {
        if (player->bullets[i].active) {
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (enemies[j].active &&
                    player->bullets[i].x == enemies[j].x &&
                    (player->bullets[i].y == enemies[j].y ||
                    player->bullets[i].y == enemies[j].y - 1 ||
                    player->bullets[i].y == enemies[j].y + 1)
                    ) {
                    player->bullets[i].active = 0; // 子弹失效
                    enemies[j].active = 0;  // 敌机销毁
                    player->score++;        // 得分增加
                }
            }
        }
    }
}

// 检测玩家是否被敌机碰撞
int check_player_collision(Player *player, Enemy* enemies, int client_sock) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active &&
            enemies[i].x == player->x &&
            enemies[i].y == player->y) {
            player->x = -1;
            // 发送 Ctrl+C 信号给客户端
            char ctrl_c = SIGINT_CHAR;
            send(client_sock, &ctrl_c, sizeof(ctrl_c), 0);
            return 1; // 玩家被敌机撞到
        }
    }
    return 0;
}

int update(int game_over, Enemy *enemies, int client_sock) {
        // 生成敌机
        if (rand() % 20 == 0) {
            spawn_enemy(enemies);
        }

        // 更新状态
        for (int i = 0; i < MAX_PLAYERS; i++) {
            update_bullets(&players[i]);
            check_collision(&players[i],enemies);
        }
        update_enemies(enemies);
        for (int i = 0; i < MAX_PLAYERS; i++) {
            game_over |= check_player_collision(&players[i], enemies, client_sock);
        }

        return game_over;
}

// 将数据封装为message
Message toMessage(Player *players, Enemy *enemies, int number, int index) {
    Message message;
    message.players = players;   // 直接引用传入的 players
    message.enemies = enemies;   // 直接引用传入的 enemies
    message.player_number = number;
    message.index = index;
    return message;
}

// 序列化函数
void serialize_message(Message *message, char *buffer) {
    int offset = 0;

    // 复制 player_number 和 index 到 buffer
    memcpy(buffer + offset, &message->player_number, sizeof(int));
    offset += sizeof(int);
    memcpy(buffer + offset, &message->index, sizeof(int));
    offset += sizeof(int);

    // 序列化 players 数据
    for (int i = 0; i < message->player_number; i++) {
        Player *player = &message->players[i];

        // 序列化玩家数据
        memcpy(buffer + offset, &player->x, sizeof(int));
        offset += sizeof(int);
        memcpy(buffer + offset, &player->y, sizeof(int));
        offset += sizeof(int);
        memcpy(buffer + offset, &player->score, sizeof(int));
        offset += sizeof(int);
        memcpy(buffer + offset, &player->color_pair, sizeof(int));
        offset += sizeof(int);
        memcpy(buffer + offset, &player->bullet_color, sizeof(int));
        offset += sizeof(int);
        memcpy(buffer + offset, &player->max_bullets, sizeof(int));
        offset += sizeof(int);

        // 序列化子弹数据
        for (int j = 0; j < player->max_bullets; j++) {
            Bullet *bullet = &player->bullets[j];
            memcpy(buffer + offset, &bullet->x, sizeof(int));
            offset += sizeof(int);
            memcpy(buffer + offset, &bullet->y, sizeof(int));
            offset += sizeof(int);
            memcpy(buffer + offset, &bullet->active, sizeof(int));
            offset += sizeof(int);
        }
    }

    // 序列化 enemies 数据
    for (int i = 0; i < message->player_number; i++) {
        Enemy *enemy = &message->enemies[i];
        memcpy(buffer + offset, &enemy->x, sizeof(int));
        offset += sizeof(int);
        memcpy(buffer + offset, &enemy->y, sizeof(int));
        offset += sizeof(int);
        memcpy(buffer + offset, &enemy->active, sizeof(int));
        offset += sizeof(int);
    }
}