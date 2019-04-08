#include <stdbool.h>

#define SCREEN_X 320
#define SCREEN_Y 240

#define BOARD_X 80
#define BOARD_Y 60

#define NUM_PLAYERS 4

typedef struct Player_State{
  //player position
  int pos_x;
  int pos_y;

  //player direction/speed
  int dx;
  int dy;

  //player value on game_board
  int value;

  //player colour
  short int color;

  bool is_alive;
} Player_State;

void config_PS2(void);

void update_score(int score);

//sets all pixels in the screen to black
void clear_screen();

//waits for the screen to refresh
void wait_for_vsync();

//draws the board onto the screen
void draw_board(int game_board[BOARD_X][BOARD_Y], Player_State players[4]);

//draws a line, using Bresenham's algorithm
void draw_line(int x0, int y0, int  x1, int y1, short int line_color);

//draws a rectangle starting from the top left corner
void draw_rectangle(int x, int y, int width, int height, short int color);

//plots a pixel
void plot_pixel(int x, int y, short int color);

//swap two integers
void swap(int* x, int* y);

void kill_player(int player, int game_board[BOARD_X][BOARD_Y]);
