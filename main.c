#include <stdbool.h>

extern void print(const char *);
extern void enable_interrupt(void);

extern float sin[360];
extern float cos[360];

#define TICKS_PER_SEC 20

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define BALL_RADIUS 2
#define HIT_COOLDOWN 10
#define BALL_SPEED 2

#define PADDLE_RADIUS SCREEN_HEIGHT / 2 - 5
#define PADDLE_WIDTH_DEG 30
#define PADDLE_DIST_FROM_MIDDLE 110
#define PADDLE_MOVEMENT_SPEED 2

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
#define C_RED 0b11100000
#define C_BLUE 0b00000111

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
    int score[2];   // 0: player one, 1: player two
    int last_touch; // 0: player one, 1: player two

    float ball_pos_x;
    float ball_pos_y;
    float ball_vel_x;
    float ball_vel_y;

    int hit_cooldown;

    Paddle p_one;
    Paddle p_two;

} Game;

Game gamestate;

void setup_timer()
{
    int *timer_p = (int *)0x04000020;
    *(timer_p + 3) = 0xF;    // Periodh
    *(timer_p + 2) = 0x4240; // Periodl
    *(timer_p + 1) = 0x7;    // Control
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

void draw_circle(int x, int y, int radius, int color)
{
    // Jesko's method variant of midpoint circle algorithm
    int t1 = radius / 16;
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

void draw_score(int score[2])
{
    SEGMENT_DISPLAY[2] = digits[score[0] % 10];
    SEGMENT_DISPLAY[3] = 0b10000000 | digits[score[1] % 10];
}

void draw_screen(Game *game)
{
    draw_circle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, PADDLE_RADIUS, C_WHITE);
    game->p_one.color = C_BLUE;
    game->p_two.color = C_RED;
    draw_paddle(game->p_one);
    draw_paddle(game->p_two);
    draw_circle(game->ball_pos_x, game->ball_pos_y, BALL_RADIUS, C_WHITE);
}

void clear_screen(Game *game)
{
    game->p_one.color = C_BLACK;
    game->p_two.color = C_BLACK;
    draw_paddle(game->p_one);
    draw_paddle(game->p_two);
    draw_circle(game->ball_pos_x, game->ball_pos_y, BALL_RADIUS, C_BLACK);
}

int get_switches(void)
{
    volatile int *p = (int *)0x04000010;
    return *p & 0b1111111111;
}

void update_paddle_ends(int dir, Paddle *paddle)
{
    paddle->angle += dir * PADDLE_MOVEMENT_SPEED;

    int paddle_end_1 = ((paddle->angle + PADDLE_WIDTH_DEG / 2) % 360 + 360) % 360;
    int paddle_end_2 = ((paddle->angle - PADDLE_WIDTH_DEG / 2) % 360 + 360) % 360;

    paddle->ends[0].x = (PADDLE_RADIUS)*cos[paddle_end_1] + (SCREEN_WIDTH) / 2;
    paddle->ends[0].y = (PADDLE_RADIUS)*sin[paddle_end_1] + (SCREEN_HEIGHT) / 2;
    paddle->ends[1].x = (PADDLE_RADIUS)*cos[paddle_end_2] + (SCREEN_WIDTH) / 2;
    paddle->ends[1].y = (PADDLE_RADIUS)*sin[paddle_end_2] + (SCREEN_HEIGHT) / 2;
}

void move_paddles(Game *game)
{
    int switches = get_switches();
    int sw0 = switches & 1;
    int sw9 = switches & 0x100;
    sw9 = sw9 >> 8;

    sw0 = sw0 == 0 ? -1 : 1;
    sw9 = sw9 == 0 ? -1 : 1;

    update_paddle_ends(sw0, &game->p_one);
    update_paddle_ends(sw9, &game->p_two);
}

void move_ball(Game *game)
{
    game->ball_pos_x += game->ball_vel_x;
    game->ball_pos_y += game->ball_vel_y;
}

bool handle_paddle_collision(Game *game, Paddle player)
{
    float px1 = player.ends[0].x;
    float py1 = player.ends[0].y;
    float px2 = player.ends[1].x;
    float py2 = player.ends[1].y;

    // Vector from paddle end to ball
    float ax = game->ball_pos_x - px1;
    float ay = game->ball_pos_y - py1;

    float bx = px2 - px1;
    float by = py2 - py1;

    // Project a on b to find nearest point to ball
    float k = (ax * bx + ay * by) / (float)(bx * bx + by * by);

    float nearestx = px1 + k * bx;
    float nearesty = py1 + k * by;

    float vx = game->ball_pos_x - nearestx;
    float vy = game->ball_pos_y - nearesty;
    float dist = vx * vx + vy * vy;

    // Collision detected if nearest point is within ball radius and on paddle
    if (dist <= BALL_RADIUS * BALL_RADIUS)
    {
        // Update ball trajectory by projecting the velocity of the ball onto
        // the normal of the paddle and subtracting the resulting vector twice
        float nx = -cos[player.angle];
        float ny = -sin[player.angle];

        float c = game->ball_vel_x * nx + game->ball_vel_y * ny;

        game->ball_vel_x -= 2 * c * nx;
        game->ball_vel_y -= 2 * c * ny;

        return true;
    }

    return false;
}

bool handle_oob_collision(Game *game)
{
    int bx = game->ball_pos_x - SCREEN_WIDTH / 2;
    int by = game->ball_pos_y - SCREEN_HEIGHT / 2;

    if (bx * bx + by * by >= (PADDLE_RADIUS - BALL_RADIUS) * (PADDLE_RADIUS - BALL_RADIUS))
    {
        game->ball_pos_x = SCREEN_WIDTH / 2;
        game->ball_pos_y = SCREEN_HEIGHT / 2;

        game->score[0] += game->last_touch == 0;
        game->score[1] += game->last_touch == 1;

        draw_score(game->score);

        return true;
    }
    return false;
}

void handle_collisions(Game *game)
{
    if (game->hit_cooldown == 0)
    {
        if (handle_paddle_collision(game, game->p_one) || handle_paddle_collision(game, game->p_two))
        {
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
        clear_screen(&gamestate);

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
    p1.angle = 0;
    p1.color = C_BLUE;

    Paddle p2;
    p2.angle = 180;
    p2.color = C_RED;

    Game game;

    game.score[0] = 0;
    game.score[1] = 0;

    game.ball_pos_x = SCREEN_WIDTH / 2;
    game.ball_pos_y = SCREEN_HEIGHT / 2;
    game.ball_vel_x = BALL_SPEED;
    game.ball_vel_y = 0;

    game.hit_cooldown = HIT_COOLDOWN;

    game.p_one = p1;
    game.p_two = p2;

    for (int i = 0; i < 6; i++)
    {
        SEGMENT_DISPLAY[i] = 0;
    }

    move_paddles(&game);
    draw_score(game.score);
    setup_timer();
    enable_interrupt();

    return game;
}

int main()
{
    gamestate = init();
    while (1)
    {
    }

    return 0;
}
