#define _GNU_SOURCE
#include <math.h>

#define TICKS_PER_SEC 20

#define N_ANGLES 360
float sin[N_ANGLES];
float cos[N_ANGLES];

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define BALL_RADIUS 10

#define PADDLE_RADIUS SCREEN_WIDTH / 2 - 10
#define PADDLE_WIDTH_DEG 10
#define PADDLE_THICKNESS 10
#define PADDLE_DIST_FROM_MIDDLE 200
Point base_paddle[2];

#define C_WHITE -1
#define C_GRAY 0b00100101

typedef struct
{
    int x;
    int y;
} Point;

typedef struct
{
    short color;
    int angle;

    Point ends[2]; // When paddle is on the right, 0: top, 1: bottom
} Paddle;

typedef struct
{
    int score[2]; // 0: player one, 1: player two

    Point ball_pos;
    Point ball_vel;

    Paddle p_one;
    Paddle p_two;
} Game;

Game init()
{
    for (int i = 0; i < N_ANGLES; i++)
    {
        float radians = i * (M_PIf / 180.0);
        sin[i] = sinf(radians);
        cos[i] = cosf(radians);
    }

    base_paddle[0] = (Point){
        .x = PADDLE_DIST_FROM_MIDDLE * cos[PADDLE_WIDTH_DEG / 2],
        .y = PADDLE_DIST_FROM_MIDDLE * sin[PADDLE_WIDTH_DEG / 2]};
    base_paddle[1] = (Point){
        .x = PADDLE_DIST_FROM_MIDDLE * cos[PADDLE_WIDTH_DEG / 2],
        .y = PADDLE_DIST_FROM_MIDDLE * -sin[PADDLE_WIDTH_DEG / 2]};

    Paddle p1 = {
        .angle = 0,
        .color = C_WHITE,
        .ends = {base_paddle[0], base_paddle[1]}};
    Paddle p2 = {
        .angle = 180,
        .color = C_GRAY,
        .ends = {(Point){.x = -base_paddle[0].x, .y = -base_paddle[0].y},
                 (Point){.x = -base_paddle[1].x, .y = -base_paddle[1].y}}};

    Game game = {
        .score = {0, 0},
        .ball_pos = {0, 0},
        .ball_vel = {1, 0}};

    // Draw game outside circle
    // midpoint circle algo

    return game;
}

void tick()
{
    // Set timer
    // Wait interrupt
}

void draw_screen(Game game)
{
    // Bresenham's line algo
}

int get_switches(void)
{
    volatile int *p = (int *)0x04000010;
    return *p & 0b1111111111;
}

void move_paddles(Game game)
{
    int switches = get_switches();
    int sw0 = switches & 1;
    int sw9 = switches & 0x100;

    sw0 = sw0 | (-1 & ~sw0);
    sw9 = sw9 | (-1 & ~sw9);

    game.p_one.angle += sw0;
    game.p_two.angle += sw9;

    game.p_one.ends[0].x = PADDLE_DIST_FROM_MIDDLE * cos[game.p_one.angle + PADDLE_WIDTH_DEG / 2];
    game.p_one.ends[0].y = PADDLE_DIST_FROM_MIDDLE * sin[game.p_one.angle + PADDLE_WIDTH_DEG / 2];
    game.p_one.ends[1].x = PADDLE_DIST_FROM_MIDDLE * cos[game.p_one.angle - PADDLE_WIDTH_DEG / 2];
    game.p_one.ends[1].y = PADDLE_DIST_FROM_MIDDLE * sin[game.p_one.angle - PADDLE_WIDTH_DEG / 2];
}

void move_ball(Game game)
{
    game.ball_pos.x += game.ball_vel.x;
    game.ball_pos.y += game.ball_vel.y;
}

void check_collision(Game game)
{
    int dx = game.p_one.ends[0].x - game.p_one.ends[1].x;
    int dy = game.p_one.ends[0].y - game.p_one.ends[1].y;

    int fx = game.ball_pos.x - game.p_one.ends[0].x;
    int fy = game.ball_pos.y - game.p_one.ends[0].y;
}

int main()
{
    Game state = init();

    while (1)
    {
        draw_screen(state);
        tick();
        move_paddles(state);
        move_ball(state);
        check_collision(state);
    }
}
