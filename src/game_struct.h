// 软件2201 苏一行 20229639
#ifndef PERSON_H
#define PERSON_H

#define DELAY 100000000L  // 控制游戏速度（纳秒）
#define MAX_ENEMIES 10 // 最多敌机数量
#define MAX_PLAYERS 100   // 最大玩家数量

// 全局变量
int max_x, max_y;           // 终端屏幕的宽度和高度

typedef struct {    // 子弹结构体
    int x, y;   // 子弹位置
    int active; // 子弹是否有效
} Bullet;

typedef struct {    // 敌机结构体
    int x, y;
    int active;
} Enemy;

typedef struct {    // 玩家结构体
    int x, y;           // 玩家位置
    int score;          // 玩家得分
    int color_pair;     // 玩家颜色对
    int bullet_color;   // 子弹颜色对
    int max_bullets;    // 最大子弹数量
    Bullet *bullets;    // 子弹数组
} Player;

// TODO 数据传输
typedef struct {        // 服务端传输给客户端的数据包
    Player *players;    // 玩家数据
    Enemy *enemies;     // 敌机数组
    int player_number;  // 玩家数量
    int index;          // 客户端序号
} Message;

#endif


/*
1.5
总结：
服务端以及可以向客户端传输完整的数据了

下一步：
客户端对颜进色行初始化
客户端对元素进行渲染
接收客户端的输入，传递给服务端
服务端接收客户端数据，并改变飞机位置或发射子弹
*/

/*
1.6
总结：
客户端对服务端数据的渲染
改变飞机位置，发射子弹，击落敌机，增加分数

下一步：
对每个客户端实现被敌机击落的处理
修复客户端1发射子弹的小bug
（实现登录注册）
*/


/*
1.7
实现了每个客户端被击落的处理

下一步：
修改链接多个客户端异常退出的bug
对性能进行优化
撰写实验报告并录制答辩视频
*/