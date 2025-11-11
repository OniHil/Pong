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
#define PADDLE_MOVEMENT_SPEED 1
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

    return game;
}

void tick()
{
    // Set timer
    // Wait interrupt
}

void draw(int x, int y, int color)
{
    volatile char *VGA = (volatile char *)0x08000000;
    VGA[x + y * SCREEN_WIDTH] = color;
}

void draw_game_circle()
{
    // midpoint circle algo
}

void draw_paddle(Paddle paddle)
{
    // Bresenham's line algo https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
    int x0 = paddle.ends[0].x;
    int y0 = paddle.ends[0].y;
    int x1 = paddle.ends[1].x;
    int y1 = paddle.ends[1].y;

    int dx = x1 - x0;
    int nx = dx >> 31;
    dx = (dx + nx) ^ nx;

    int dy = y1 - y0;
    int ny = dy >> 31;
    dy = -((dy + ny) ^ ny);

    int err = dx + dy;

    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;

    int total_steps = (dx > -dy) ? dx : -dy;

    for (int i = 0; i <= total_steps; i++)
    {
        draw(x0, y0, paddle.color);

        int e2 = err << 1;
        int move_x = e2 >= dy;
        int move_y = e2 <= dx;

        err += dy * move_x + dx * move_y;
        x0 += sx * move_x;
        y0 += sy * move_y;
    }
}

void draw_ball(Point pos)
{
}

void draw_score(int score[2])
{
}

void draw_screen(Game game)
{
    draw_game_circle();
    draw_paddle(game.p_one);
    draw_paddle(game.p_two);
    draw_ball(game.ball_pos);
    draw_score(game.score);
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

    sw0 = (PADDLE_MOVEMENT_SPEED & sw0) | (-PADDLE_MOVEMENT_SPEED & ~sw0);
    sw9 = (PADDLE_MOVEMENT_SPEED & sw9) | (-PADDLE_MOVEMENT_SPEED & ~sw9);

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

    return 0;
}
