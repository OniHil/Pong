#include <stdbool.h>
extern float sin[360];
extern float cos[360];

#define TICKS_PER_SEC 20

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define BALL_RADIUS 2

#define PADDLE_RADIUS SCREEN_HEIGHT / 2 - 5
#define PADDLE_WIDTH_DEG 10
#define PADDLE_THICKNESS 10
#define PADDLE_DIST_FROM_MIDDLE 50
#define PADDLE_MOVEMENT_SPEED 1

#define C_BLACK 0
#define C_WHITE -1
#define C_GRAY 0b00100101

typedef struct
{
    int x;
    int y;
} Point;

Point base_paddle[2];

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
    base_paddle[0].x = PADDLE_DIST_FROM_MIDDLE * cos[PADDLE_WIDTH_DEG / 2],
    base_paddle[0].y = PADDLE_DIST_FROM_MIDDLE * sin[PADDLE_WIDTH_DEG / 2];
    base_paddle[1].x = PADDLE_DIST_FROM_MIDDLE * cos[PADDLE_WIDTH_DEG / 2],
    base_paddle[1].y = PADDLE_DIST_FROM_MIDDLE * -sin[PADDLE_WIDTH_DEG / 2];

    Paddle p1;
    p1.angle = 0;
    p1.color = C_WHITE;
    p1.ends[0] = base_paddle[0];
    p1.ends[1] = base_paddle[1];
    
    Paddle p2;
    p2.angle = 180;
    p2.color = C_GRAY;
    p2.ends[0].x = -base_paddle[0].x;
    p2.ends[0].y = -base_paddle[0].y;
    p2.ends[1].x = -base_paddle[1].x;
    p2.ends[1].y = -base_paddle[1].y;
    
    Game game;
    
    game.score[0] = 0;
    game.score[1] = 0;
    
    game.ball_pos.x = SCREEN_WIDTH / 2;
    game.ball_pos.y = SCREEN_HEIGHT / 2;
    game.ball_vel.x = 1;
    game.ball_vel.y = 0;

    game.p_one = p1;
    game.p_two = p2;

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
    //Jesko's method variant of midpoint circle algorithm
    int t1 = radius / 16;
    int t2 = 0;
    int dx = radius;
    int dy = 0;

    while(dx >= dy) {
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
        if (t2 >= 0) {
            t1 = t2;
            dx--;
        }
    }
}

void draw_score(int score[2])
{
}

void draw_screen(Game game)
{
    draw_circle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, PADDLE_RADIUS, C_WHITE);
    draw_paddle(game.p_one);
    draw_paddle(game.p_two);
    draw_circle(game.ball_pos.x, game.ball_pos.y, BALL_RADIUS, C_WHITE);
    // draw_score(game.score);
}

int get_switches(void)
{
    volatile int *p = (int *)0x08000000;
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

void move_ball(Game* game)
{
    game->ball_pos.x += game->ball_vel.x;
    game->ball_pos.y += game->ball_vel.y;
}

bool handle_paddle_collision(Game game, Paddle player)
{
    int px1 = player.ends[0].x;
    int py1 = player.ends[0].y;
    int px2 = player.ends[1].x;
    int py2 = player.ends[1].y;

    // Vector from paddle end to ball
    int ax = game.ball_pos.x - px1;
    int ay = game.ball_pos.y - py1;   
    
    int bx = px2 - px1;
    int by = py2 - py1;

    // Project a on b to find nearest point to ball
    float k = (float)(ax * bx + ay * by) / (bx * bx + by * by);

    float nearestx = px1 + k * bx;
    float nearesty = py1 + k * by;

    float vx = bx - nearestx;
    float vy = by - nearesty;
    float dist = vx * vx + vy * vy;
    
    // Collision detected if nearest point is within ball radius and on paddle
    if (dist <= BALL_RADIUS * BALL_RADIUS && k <= 1 && k >= 0) {
        // Update ball trajectory by projecting the velocity of the ball onto
        // the normal of the paddle and subtracting the resulting vector twice
        float nx = -cos[player.angle];
        float ny = -sin[player.angle];

        int velx = game.ball_vel.x;
        int vely = game.ball_vel.y;

        float c = velx * nx + vely * ny;
        game.ball_vel.x = velx - 2 * c * nx;
        game.ball_vel.y = vely - 2 * c * ny;

        return true;
    }

    return false;
}

bool handle_oob_collision(Game* game) {
    int bx = game->ball_pos.x - SCREEN_WIDTH / 2;
    int by = game->ball_pos.y - SCREEN_HEIGHT / 2;
    
    if (bx * bx + by * by >= (PADDLE_RADIUS - BALL_RADIUS) * (PADDLE_RADIUS - BALL_RADIUS)) {
        game->ball_pos.x = SCREEN_WIDTH / 2;
        game->ball_pos.y = SCREEN_HEIGHT / 2;
        game->ball_vel.y += 1;

        return true;
    }
    return false;
}

void handle_collisions(Game* game) {
    //handle_paddle_collision(game, game.p_one);
    //handle_paddle_collision(game, game.p_two);
    handle_oob_collision(game);
}

int main()
{
    Game state = init();

    draw_screen(state);
    while (1)
    {
        // Wait and draw new state
        tick();
        draw_screen(state);

        // Reset drawings
        state.p_one.color = C_BLACK;
        state.p_two.color = C_BLACK;
        draw_paddle(state.p_one);
        draw_paddle(state.p_two);

        // Calculate new state
        move_paddles(state);
        move_ball(&state);
        handle_collisions(&state);
    }

    return 0;
}
