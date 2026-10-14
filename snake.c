#include <curses.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TIMEOUT_MS  150
#define GRID_WIDTH  20
#define GRID_HEIGHT 20
#define GRID_SIZE   (GRID_WIDTH * GRID_HEIGHT)
#define CHAR_EMPTY  '.'
#define CHAR_HEAD   '0'
#define CHAR_BODY   'o'
#define CHAR_APPLE  '@'
#define PADDING_X   1
#define PADDING_Y   1

typedef struct {
	int x;
	int y;
} vec2;

WINDOW *window;
int score;
int highscore;
int snake_size;
int tail_idx;
int head_idx;
vec2 prev_tail_pos;
vec2 prev_head_pos;
vec2 snake[GRID_SIZE];
vec2 dir;
vec2 next_head_pos;
vec2 apple;
bool snake_collided;
bool apple_consumed;
bool occupied[GRID_HEIGHT][GRID_WIDTH];
bool finished;

void init_window(void);
void init_state(void);
void update_apple_pos(void);
void init_drawing(void);
void draw_char(int y, int x, char ch);
void print_size(void);
bool read_input(void);
void update_state(void);
void update_drawing(void);

int main(void)
{
	highscore = 0;

	setlocale(LC_ALL, "");
	srand((unsigned)time(NULL));

	init_window();
	init_state();
	init_drawing();

	for (;;) {
		if (!read_input())
			break;

		update_state();
		update_drawing();

		if (snake_collided || finished) {
			highscore = score > highscore ? score : highscore;
			wtimeout(window, -1);

			if (!read_input())
				break;

			wtimeout(window, TIMEOUT_MS);
			init_state();
			init_drawing();
		}
	}

	endwin();

	printf("score: %3d   highscore: %3d\n", score, highscore);

	return 0;
}

void init_window(void)
{
	window = initscr();
	cbreak();
	noecho();
	keypad(window, TRUE);
	wtimeout(window, TIMEOUT_MS);
	set_escdelay(25);
	curs_set(0);
}

void init_state(void)
{
	int x, y;

	score = 0;
	snake_size = 2;
	tail_idx = 0;
	head_idx = 1;
	snake[tail_idx].x = GRID_WIDTH / 4;
	snake[tail_idx].y = GRID_HEIGHT / 2;
	snake[head_idx].x = snake[tail_idx].x + 1;
	snake[head_idx].y = snake[tail_idx].y;
	dir.x = 1;
	dir.y = 0;

	for (y = 0; y < GRID_HEIGHT; y++)
		for (x = 0; x < GRID_WIDTH; x++)
			occupied[y][x] = false;

	occupied[snake[tail_idx].y][snake[tail_idx].x] = true;
	occupied[snake[head_idx].y][snake[head_idx].x] = true;

	update_apple_pos();
}

void update_apple_pos(void)
{
	do {
		apple.x = rand() % GRID_WIDTH;
		apple.y = rand() % GRID_HEIGHT;
	} while (occupied[apple.y][apple.x]);
}

void init_drawing(void)
{
	int x, y;

	werase(window);

	for (y = 0; y < GRID_HEIGHT; y++)
		for (x = 0; x < GRID_WIDTH; x++)
			draw_char(y, x, CHAR_EMPTY);

	draw_char(snake[tail_idx].y, snake[tail_idx].x, CHAR_BODY);
	draw_char(snake[head_idx].y, snake[head_idx].x, CHAR_HEAD);
	draw_char(apple.y, apple.x, CHAR_APPLE);

	print_size();
}

void draw_char(int y, int x, char ch)
{
	mvwaddch(window, PADDING_Y + y, (PADDING_X + x) * 2, ch);
}

void print_size(void)
{
	mvwprintw(window, PADDING_Y + GRID_HEIGHT, PADDING_X * 2,
	          "score: %3d   highscore: %3d", score, highscore);
}

bool read_input(void)
{
	int pressed = wgetch(window);

	switch (pressed) {
		case 'w':
			if (dir.y != 1) {
				dir.x = 0;
				dir.y = -1;
			}
			break;
		case 'a':
			if (dir.x != 1) {
				dir.x = -1;
				dir.y = 0;
			}
			break;
		case 's':
			if (dir.y != -1) {
				dir.x = 0;
				dir.y = 1;
			}
			break;
		case 'd':
			if (dir.x != -1) {
				dir.x = 1;
				dir.y = 0;
			}
			break;
		case '\033':
			return false;
			break;
	}

	return true;
}

void update_state(void)
{
	next_head_pos.x = snake[head_idx].x + dir.x;
	next_head_pos.y = snake[head_idx].y + dir.y;

	snake_collided = false;
	if (next_head_pos.x < 0 || next_head_pos.x >= GRID_WIDTH ||
	    next_head_pos.y < 0 || next_head_pos.y >= GRID_HEIGHT)
		snake_collided = true;
	else if (occupied[next_head_pos.y][next_head_pos.x] &&
	        (next_head_pos.x != snake[tail_idx].x ||
	         next_head_pos.y != snake[tail_idx].y))
		snake_collided = true;

	apple_consumed = next_head_pos.x == apple.x &&
	                 next_head_pos.y == apple.y;

	prev_tail_pos = snake[tail_idx];
	prev_head_pos = snake[head_idx];

	if (snake_collided)
		return;

	if (!apple_consumed) {
		occupied[prev_tail_pos.y][prev_tail_pos.x] = false;
		tail_idx = (tail_idx + 1) % GRID_SIZE;
	}

	occupied[next_head_pos.y][next_head_pos.x] = true;
	head_idx = (head_idx + 1) % GRID_SIZE;

	if (apple_consumed) {
		score += 1;
		snake_size += 1;
		finished = snake_size == GRID_SIZE;

		if (!finished)
			update_apple_pos();
	}

	snake[head_idx] = next_head_pos;
}

void update_drawing(void)
{
	if (!apple_consumed)
		draw_char(prev_tail_pos.y, prev_tail_pos.x, CHAR_EMPTY);

	draw_char(prev_head_pos.y, prev_head_pos.x, CHAR_BODY);
	draw_char(next_head_pos.y, next_head_pos.x, CHAR_HEAD);

	if (apple_consumed) {
		if (!finished)
			draw_char(apple.y, apple.x, CHAR_APPLE);
		print_size();
	}
}
