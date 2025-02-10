#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include "game_struct.h"

Message message;

// 初始化
void init_ncurses(){
    srand(time(NULL)); // 初始化随机数种子

    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE); // 启用键盘输入
}
// 初始化颜色
void init_colors(Message message) {
    start_color();
    // for (int i = 0; i < message.player_number; i++){
    //     init_pair(1, message.players[i].color_pair + 2, COLOR_BLACK);    // 玩家index颜色
    // }
    init_pair(1, COLOR_BLUE, COLOR_BLACK);   // 玩家1颜色
    init_pair(2, COLOR_MAGENTA, COLOR_BLACK);   // 玩家2颜色
    init_pair(3, COLOR_RED, COLOR_BLACK);   // 玩家3颜色
    init_pair(3, COLOR_GREEN, COLOR_BLACK);  // 子弹颜色
    init_pair(4, COLOR_YELLOW, COLOR_BLACK); // 敌机颜色
    init_pair(5, COLOR_WHITE, COLOR_BLACK); // 得分颜色
}

// 显示玩家飞机
void draw_player(Player *player) {
    if (player->x == -1) return;
    attron(COLOR_PAIR(player->color_pair));
    mvprintw(player->y, player->x, "A");
    attroff(COLOR_PAIR(player->color_pair));
}

// 显示子弹
void draw_bullets(Player *player) {
    attron(COLOR_PAIR(player->bullet_color));
    for (int i = 0; i < player->max_bullets; i++) {
        if (player->bullets[i].active) {
            mvprintw(player->bullets[i].y, player->bullets[i].x, "|");
        }
    }
    attroff(COLOR_PAIR(player->bullet_color));
}

// 显示敌机
void draw_enemies(Message message) {
    Enemy *enemies = message.enemies;
    attron(COLOR_PAIR(4));
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            mvprintw(enemies[i].y, enemies[i].x, "V");
        }
    }
    attroff(COLOR_PAIR(4));
}

void get_terminal_xy() {
    struct winsize w;
    // 获取终端大小
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        exit(EXIT_FAILURE);
    }
    max_y = w.ws_row;
    max_x = w.ws_col;
}

// 显示每个玩家的得分
void draw_socre(Message message) {
    get_terminal_xy();
    for (int i = 0; i < message.player_number; i++) {
        attron(COLOR_PAIR(message.players[i].color_pair));
        mvprintw(1 * i, max_x - 20, "Player %d score is %d", i + 1, message.players[i].score);
        attroff(COLOR_PAIR(message.players[i].bullet_color));
    }

}

int showGame(Message message) { // 刷新玩家以及敌机
    clear();
    Player *players = message.players;

    // 绘制游戏元素
    for (int i = 0; i < message.player_number; i++) {
        draw_player(&players[i]);
        draw_bullets(&players[i]);
    }
    draw_enemies(message);
    draw_socre(message);

    // 刷新屏幕
    refresh();
}

void draw_game_over(int score){
    clear();
    mvprintw(max_y / 2, max_x / 2 - 5, "Game Over!");
    mvprintw(max_y / 2 + 2, max_x / 2 - 7, "your score is %d", score);
    refresh();
    sleep(3);
}

void showGameDown(int score){
    refresh();
    clear();
    attron(COLOR_PAIR(5));
    mvprintw(max_y / 2, max_x / 2 - 10, "The Game Over , You Score is :", score);
    attroff(COLOR_PAIR(5));

    sleep(20);
}

// TODO 反序列化函数
void deserialize_message(char *buffer, Message *message) {
    int offset = 0;

    // 读取 player_number 和 index
    memcpy(&message->player_number, buffer + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&message->index, buffer + offset, sizeof(int));
    offset += sizeof(int);

    // 为 players 和 enemies 分配内存
    message->players = malloc(message->player_number * sizeof(Player));
    message->enemies = malloc(message->player_number * sizeof(Enemy));

    // 反序列化 players 数据
    for (int i = 0; i < message->player_number; i++) {
        Player *player = &message->players[i];

        // 反序列化玩家数据
        memcpy(&player->x, buffer + offset, sizeof(int));
        offset += sizeof(int);
        memcpy(&player->y, buffer + offset, sizeof(int));
        offset += sizeof(int);
        memcpy(&player->score, buffer + offset, sizeof(int));
        offset += sizeof(int);
        memcpy(&player->color_pair, buffer + offset, sizeof(int));
        offset += sizeof(int);
        memcpy(&player->bullet_color, buffer + offset, sizeof(int));
        offset += sizeof(int);
        memcpy(&player->max_bullets, buffer + offset, sizeof(int));
        offset += sizeof(int);

        // 反序列化子弹数据
        player->bullets = malloc(player->max_bullets * sizeof(Bullet));
        for (int j = 0; j < player->max_bullets; j++) {
            Bullet *bullet = &player->bullets[j];
            memcpy(&bullet->x, buffer + offset, sizeof(int));
            offset += sizeof(int);
            memcpy(&bullet->y, buffer + offset, sizeof(int));
            offset += sizeof(int);
            memcpy(&bullet->active, buffer + offset, sizeof(int));
            offset += sizeof(int);
        }
    }

    // 反序列化 enemies 数据
    for (int i = 0; i < message->player_number; i++) {
        Enemy *enemy = &message->enemies[i];
        memcpy(&enemy->x, buffer + offset, sizeof(int));
        offset += sizeof(int);
        memcpy(&enemy->y, buffer + offset, sizeof(int));
        offset += sizeof(int);
        memcpy(&enemy->active, buffer + offset, sizeof(int));
        offset += sizeof(int);
    }
}