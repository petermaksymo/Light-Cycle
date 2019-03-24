#define SCREEN_X 320
#define SCREEN_Y 240

#define BOARD_X 80
#define BOARD_Y 60

#define NUM_PLAYERS 4

//sets all pixels in the screen to black
void clear_screen();

//waits for the screen to refresh
void wait_for_vsync();

//draws the board onto the screen
void draw_board(int game_board[BOARD_X][BOARD_Y]);

//draws a line, using Bresenham's algorithm
void draw_line(int x0, int y0, int  x1, int y1, short int line_color);

//draws a rectangle starting from the top left corner
void draw_rectangle(int x, int y, int width, int height, short int color);

//plots a pixel
void plot_pixel(int x, int y, short int color);

//swap two integers
void swap(int* x, int* y);

//pushbutton interrupt service routine
void pushbutton_ISR (void);
