#include <stdbool.h>

extern void print(const char *);
extern void print_dec(unsigned int);

extern float sin[360];
extern float cos[360];

#define TICKS_PER_SEC 20

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define BALL_RADIUS 2
#define HIT_COOLDOWN 10
#define BALL_SPEED 2
#define SPEED_MULT 1.05

#define PADDLE_RADIUS SCREEN_HEIGHT / 2 - 5
#define PADDLE_WIDTH_DEG 30
#define PADDLE_DIST_FROM_MIDDLE 110
#define PADDLE_MOVEMENT_SPEED 2

#define VGA ((volatile char *)0x08000000)
#define SEGMENT_DISPLAY ((volatile int *)0x04000050)
const unsigned char digits[10] = {
    0b11000000,
    0b11111001,
    0b10100100,
    0b10110000,
    0b10011001,
    0b10010010,
    0b10000010,
    0b11111000,
    0b10000000,
    0b10011000};

#define C_BLACK 0
#define C_WHITE -1
#define C_GRAY 0b00100101
#define C_P1 0b00000111
#define C_P2 0b11100000

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
    float pos_x;
    float pos_y;
    float vel_x;
    float vel_y;
    int color;

} Ball;

typedef struct
{
    int score[2];   // 0: player one, 1: player two
    int last_touch; // 0: player one, 1: player two

    int hit_cooldown;

    Ball ball;
    Paddle paddles[2];

} Game;

Game gamestate;

typedef struct
{
    unsigned int mcycle;
    unsigned int minstret;
    unsigned int mhpmcounter3;
    unsigned int mhpmcounter4;
    unsigned int mhpmcounter5;
    unsigned int mhpmcounter6;
    unsigned int mhpmcounter7;
    unsigned int mhpmcounter8;
    unsigned int mhpmcounter9;

    unsigned int le;
    unsigned int mem;
    unsigned int dsp;

} Perf;

void setup_timer()
{
    int *timer_p = (int *)0x04000020;
    *(timer_p + 3) = 0xF;    // Periodh
    *(timer_p + 2) = 0x4240; // Periodl
    *(timer_p + 1) = 0x7;    // Control
}

Perf start;
Perf end;

Perf capture_perf()
{
    Perf perf;
    asm volatile("csrr %0, mcycle" : "=r"(perf.mcycle));
    asm volatile("csrr %0, minstret" : "=r"(perf.minstret));
    asm volatile("csrr %0, mhpmcounter3" : "=r"(perf.mhpmcounter3));
    asm volatile("csrr %0, mhpmcounter4" : "=r"(perf.mhpmcounter4));
    asm volatile("csrr %0, mhpmcounter5" : "=r"(perf.mhpmcounter5));
    asm volatile("csrr %0, mhpmcounter6" : "=r"(perf.mhpmcounter6));
    asm volatile("csrr %0, mhpmcounter7" : "=r"(perf.mhpmcounter7));
    asm volatile("csrr %0, mhpmcounter8" : "=r"(perf.mhpmcounter8));
    asm volatile("csrr %0, mhpmcounter9" : "=r"(perf.mhpmcounter9));

    return perf;
}

void print_perf(Perf start, Perf end)
{
    unsigned int mcycle = end.mcycle - start.mcycle;
    unsigned int minstret = end.minstret - start.minstret;
    unsigned int mhpmcounter3 = end.mhpmcounter3 - start.mhpmcounter3; // Memory instruction
    unsigned int mhpmcounter4 = end.mhpmcounter4 - start.mhpmcounter4; // I-cache miss
    unsigned int mhpmcounter5 = end.mhpmcounter5 - start.mhpmcounter5; // D-cache miss
    unsigned int mhpmcounter6 = end.mhpmcounter6 - start.mhpmcounter6; // I-cache stall
    unsigned int mhpmcounter7 = end.mhpmcounter7 - start.mhpmcounter7; // D-cache stall
    unsigned int mhpmcounter8 = end.mhpmcounter8 - start.mhpmcounter8; // Data hazard stall
    unsigned int mhpmcounter9 = end.mhpmcounter9 - start.mhpmcounter9; // ALU stall

    print("======== All numbers:\n");
    print("Cycles: ");
    print_dec(mcycle);
    print("\n");
    print("Instructions: ");
    print_dec(minstret);
    print("\n");
    print("Memory instruction: ");
    print_dec(mhpmcounter3);
    print("\n");
    print("I-cache miss: ");
    print_dec(mhpmcounter4);
    print("\n");
    print("D-cache miss: ");
    print_dec(mhpmcounter5);
    print("\n");
    print("I-cache stall: ");
    print_dec(mhpmcounter6);
    print("\n");
    print("D-cache stall: ");
    print_dec(mhpmcounter7);
    print("\n");
    print("Data hazard stall: ");
    print_dec(mhpmcounter8);
    print("\n");
    print("ALU stall: ");
    print_dec(mhpmcounter9);
    print("\n");

    print("======== Ratios:\n");
    float ipc = (float)minstret / mcycle;
    print("IPC: 0.");
    print_dec(ipc * 100);
    print("\n");

    float dcmiss_ratio = (float)mhpmcounter5 / mhpmcounter3;
    print("D-cache miss ratio: ");
    print_dec(dcmiss_ratio * 100);
    print("%\n");

    float icmiss_ratio = (float)mhpmcounter4 / minstret;
    print("I-cache miss ratio: ");
    print_dec(icmiss_ratio * 100);
    print("%\n");

    float icstall_ratio = (float)mhpmcounter6 / minstret;
    print("I-cache stall ratio: ");
    print_dec(icstall_ratio * 100);
    print("%\n");

    float alu_stall_ratio = (float)mhpmcounter9 / mcycle;
    print("ALU-stall ratio: ");
    print_dec(alu_stall_ratio * 100);
    print("%\n");

    float mem_intens = (float)mhpmcounter3 / minstret;
    print("Memory Intensity: ");
    print_dec(mem_intens * 100);
    print("%\n");

    float hazard = (float)mhpmcounter8 / mcycle;
    print("Hazard-stall ratio: ");
    print_dec(hazard * 100);
    print("%\n");

    print("Cache misses: ");
    print_dec(mhpmcounter4 + mhpmcounter5);
    print("\n");
}

inline void enable_interrupt(void)
{
    asm volatile("csrsi mstatus, 3 ");
    asm volatile("csrsi mie, 16");
}

inline void draw(int x, int y, int color)
{
    VGA[x + y * SCREEN_WIDTH] = color;
}

inline void draw_paddle(Paddle paddle)
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

inline void draw_circle(int x, int y, int radius, int color)
{
    // Jesko's method variant of midpoint circle algorithm

    int t1 = radius >> 4;
    int t2 = 0;
    int dx = radius;
    int dy = 0;

    while (dx >= dy)
    {
        draw(x + dx, y + dy, color);
        draw(x - dx, y + dy, color);
        draw(x + dx, y - dy, color);
        draw(x - dx, y - dy, color);
        draw(x + dy, y + dx, color);
        draw(x - dy, y + dx, color);
        draw(x + dy, y - dx, color);
        draw(x - dy, y - dx, color);

        dy++;
        t1 = t1 + dy;
        t2 = t1 - dx;

        if (t2 >= 0)
        {
            t1 = t2;
            dx--;
        }
    }
}

inline void draw_score(int score[2])
{
    SEGMENT_DISPLAY[0] = digits[score[0] % 10];
    SEGMENT_DISPLAY[4] = digits[score[0] / 10];
    SEGMENT_DISPLAY[16] = digits[score[1] % 10];
    SEGMENT_DISPLAY[20] = digits[score[1] / 10];
}

inline void draw_screen(Game *game)
{
    game->paddles[0].color = C_P1;
    game->paddles[1].color = C_P2;
    draw_paddle(game->paddles[0]);
    draw_paddle(game->paddles[1]);
    draw_circle(game->ball.pos_x, game->ball.pos_y, BALL_RADIUS, game->ball.color);
}

inline void clear_screen(Game game)
{
    game.paddles[0].color = C_BLACK;
    game.paddles[1].color = C_BLACK;
    draw_paddle(game.paddles[0]);
    draw_paddle(game.paddles[1]);
    draw_circle(game.ball.pos_x, game.ball.pos_y, BALL_RADIUS, C_BLACK);
}

inline int get_switches(void)
{
    volatile int *p = (int *)0x04000010;
    return *p & 0b1111111111;
}

inline void update_paddle_ends(int dir, Paddle *paddle)
{
    paddle->angle = ((paddle->angle + dir * PADDLE_MOVEMENT_SPEED) % 360 + 360) % 360;

    int paddle_end_1 = ((paddle->angle + PADDLE_WIDTH_DEG / 2) % 360 + 360) % 360;
    int paddle_end_2 = ((paddle->angle - PADDLE_WIDTH_DEG / 2) % 360 + 360) % 360;

    paddle->ends[0].x = ((PADDLE_RADIUS)*cos[paddle_end_1]) + (SCREEN_WIDTH) / 2;
    paddle->ends[0].y = ((PADDLE_RADIUS)*sin[paddle_end_1]) + (SCREEN_HEIGHT) / 2;
    paddle->ends[1].x = ((PADDLE_RADIUS)*cos[paddle_end_2]) + (SCREEN_WIDTH) / 2;
    paddle->ends[1].y = ((PADDLE_RADIUS)*sin[paddle_end_2]) + (SCREEN_HEIGHT) / 2;
}

inline void move_paddles(Game *game)
{
    int switches = get_switches();

    int sw0 = switches & 1;
    int sw9 = switches & 0x100;
    sw9 = sw9 >> 8;

    sw0 = sw0 == 0 ? -1 : 1;
    sw9 = sw9 == 0 ? -1 : 1;

    update_paddle_ends(sw0, &game->paddles[0]);
    update_paddle_ends(sw9, &game->paddles[1]);
}

inline void move_ball(Game *game)
{
    game->ball.pos_x += game->ball.vel_x;
    game->ball.pos_y += game->ball.vel_y;
}

inline bool handle_paddle_collision(Game *game, Paddle player)
{
    float px1 = player.ends[0].x;
    float py1 = player.ends[0].y;
    float px2 = player.ends[1].x;
    float py2 = player.ends[1].y;

    // Vector from paddle end to ball
    float ax = game->ball.pos_x - px1;
    float ay = game->ball.pos_y - py1;

    float bx = px2 - px1;
    float by = py2 - py1;

    // Project a on b to find nearest point to ball
    float k = (ax * bx + ay * by) / (float)(bx * bx + by * by);

    float nearestx = px1 + k * bx;
    float nearesty = py1 + k * by;

    float vx = game->ball.pos_x - nearestx;
    float vy = game->ball.pos_y - nearesty;
    float dist = vx * vx + vy * vy;

    // Collision detected if nearest point is within ball radius and on paddle
    if (dist <= BALL_RADIUS * BALL_RADIUS)
    {
        // Update ball trajectory by projecting the velocity of the ball onto
        // the normal of the paddle and subtracting the resulting vector twice
        float nx = -cos[player.angle];
        float ny = -sin[player.angle];

        float c = game->ball.vel_x * nx + game->ball.vel_y * ny;

        game->ball.vel_x -= 2 * c * nx * SPEED_MULT;
        game->ball.vel_y -= 2 * c * ny * SPEED_MULT;

        game->ball.color = player.color;

        return true;
    }

    return false;
}

inline bool handle_oob_collision(Game *game)
{
    int bx = game->ball.pos_x - SCREEN_WIDTH / 2;
    int by = game->ball.pos_y - SCREEN_HEIGHT / 2;

    if (bx * bx + by * by >= (PADDLE_RADIUS - BALL_RADIUS) * (PADDLE_RADIUS - BALL_RADIUS))
    {

        game->ball.pos_x = SCREEN_WIDTH / 2;
        game->ball.pos_y = SCREEN_HEIGHT / 2;

        int target_player = game->last_touch * -1 + 1;

        game->ball.vel_x = cos[game->paddles[target_player].angle] * BALL_SPEED;
        game->ball.vel_y = sin[game->paddles[target_player].angle] * BALL_SPEED;

        game->score[0] += game->last_touch == 0;
        game->score[1] += game->last_touch == 1;

        game->ball.color = game->paddles[game->last_touch].color;
        draw_score(game->score);

        return true;
    }
    return false;
}

inline void handle_collisions(Game *game)
{
    if (game->hit_cooldown == 0)
    {
        if (handle_paddle_collision(game, game->paddles[!game->last_touch]))
        {
            game->last_touch = !game->last_touch;
            game->hit_cooldown = HIT_COOLDOWN;
        }
        else
        {
            handle_oob_collision(game);
        }
    }
    else
    {
        game->hit_cooldown -= 1;
    }
}

void handle_interrupt(unsigned cause)
{
    switch (cause)
    {
    case 16:
        // Clears previous frame
        clear_screen(gamestate);

        // Calculates next frame
        move_paddles(&gamestate);
        move_ball(&gamestate);

        handle_collisions(&gamestate);

        // Draws next frame
        draw_screen(&gamestate);

        int *timer_p = (int *)0x04000020;
        *timer_p = 0;
        break;
    }
}

Game init()
{
    Paddle p1;
    p1.color = C_P1;
    p1.angle = 0;
    Paddle p2;
    p2.angle = 180;
    p2.color = C_P2;

    Game game;

    game.score[0] = 0;
    game.score[1] = 0;

    game.ball.pos_x = SCREEN_WIDTH / 2;
    game.ball.pos_y = SCREEN_HEIGHT / 2;
    game.ball.vel_x = BALL_SPEED;
    game.ball.vel_y = 0;

    game.paddles[0] = p1;
    game.paddles[1] = p2;

    game.last_touch = 0;
    game.hit_cooldown = HIT_COOLDOWN;
    game.ball.color = game.paddles[game.last_touch].color;

    // for (int i = 0; i < 6; i++)
    // {
    //     SEGMENT_DISPLAY[4 * i] = 0b11111111;
    // }

    SEGMENT_DISPLAY[4] = 0b11111111;
    SEGMENT_DISPLAY[8] = 0b10111111;
    SEGMENT_DISPLAY[12] = 0b10111111;
    SEGMENT_DISPLAY[16] = 0b11111111;

    move_paddles(&game);
    draw_score(game.score);
    draw_circle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, PADDLE_RADIUS, C_WHITE);
    // setup_timer();
    // enable_interrupt();

    return game;
}

int main()
{
    gamestate = init();

    start = capture_perf();
    for (int i = 0; i < 10000; i++)
    {
        handle_interrupt(16);
    }
    end = capture_perf();
    print_perf(start, end);

    while (1)
    {
    }

    return 0;
}
