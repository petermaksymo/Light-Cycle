#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "address_map_arm.h"
#include "interrupt_ID.h"
#include "defines.h"
#include "exceptions.h"
#include "main.h"

volatile int pixel_buffer_start; // global variable
int player_dx = 0;
int player_dy = 0;

int main(void) {
    disable_A9_interrupts ();	// disable interrupts in the A9 processor
    set_A9_IRQ_stack ();			// initialize the stack pointer for IRQ mode
    config_GIC ();					// configure the general interrupt controller
    config_KEYs ();				// configure pushbutton KEYs to generate interrupts

    enable_A9_interrupts ();	// enable interrupts in the A9 processor

    volatile int * pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;

    //initialize the game board, represents the screen. Each pixel here will
    //contain the player number or 0 for none
    int game_board[BOARD_X][BOARD_Y] = { 0 };

    //initialize player start positions
    int player_x[NUM_PLAYERS] = {BOARD_X/8, BOARD_X - BOARD_X/8, BOARD_X/8, BOARD_X - BOARD_X/8};
    int player_y[NUM_PLAYERS] = {BOARD_Y/8, BOARD_Y - BOARD_Y/8, BOARD_Y - BOARD_Y/8, BOARD_Y/8};

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = FPGA_PIXEL_BUF_BASE; // first store the address in the
                                        // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = SDRAM_BASE;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer

    while (1) {
        int i;
        for(i = 0; i < NUM_PLAYERS; i++) {
          game_board[ player_x[i] ][ player_y[i] ] = i + 1;
        }
        draw_board(game_board);

        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

        player_x[0] += player_dx;
        player_y[0] += player_dy;
    }
}


//clears the screen by setting all pixels to black
void clear_screen() {
  int x, y;
  for(x = 0; x < SCREEN_X; x++) {
    for(y = 0; y < SCREEN_Y; y++) {
      plot_pixel(x, y, 0x0000);
    }
  }
}


//waits for vsync
void wait_for_vsync() {
  volatile int * pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
  register int status;

  *pixel_ctrl_ptr = 1; // start the synchronization process

  status = *(pixel_ctrl_ptr + 3);
  while ((status & 0x01) != 0) {
    status = *(pixel_ctrl_ptr + 3);
  }
}


//draws the game board
void draw_board(int game_board[BOARD_X][BOARD_Y]) {
  int board_to_screen_factor = SCREEN_X / BOARD_X;
  short int player_colors[NUM_PLAYERS] = {0x07E0, 0xF800, 0x001F, 0xF81F};

  int x, y;
  for(x = 0; x < SCREEN_X; x+= board_to_screen_factor) {
    for(y = 0; y < SCREEN_Y; y+= board_to_screen_factor) {
      switch(game_board[x/board_to_screen_factor][y/board_to_screen_factor]) {
        case 0: draw_rectangle(x, y, board_to_screen_factor, board_to_screen_factor, 0x0000); break;
        case 1: draw_rectangle(x, y, board_to_screen_factor, board_to_screen_factor, player_colors[0]); break;
        case 2: draw_rectangle(x, y, board_to_screen_factor, board_to_screen_factor, player_colors[1]); break;
        case 3: draw_rectangle(x, y, board_to_screen_factor, board_to_screen_factor, player_colors[2]); break;
        case 4: draw_rectangle(x, y, board_to_screen_factor, board_to_screen_factor, player_colors[3]); break;

        default: draw_rectangle(x, y, board_to_screen_factor, board_to_screen_factor, 0x0000); break;
      }
    }
  }
}



//draws a line using Bresenham's algorithm
void draw_line(int x0, int y0, int  x1, int y1, short int line_color) {
  bool isSteep = abs(y1-y0) > abs (x1-x0);
  if (isSteep) {
    swap(&x0, &y0);
    swap(&x1, &y1);
  }
  if(x0 > x1) {
    swap(&x0, &x1);
    swap(&y0, &y1);
  }

  int dX = x1 - x0;
  int dY = abs(y1 - y0);
  int error = -(dX / 2);
  int x;
  int y = y0;
  int yStep;
  if(y0 < y1) yStep = 1;
  else yStep = -1;

  for(x = x0; x <= x1; x++) {
    if(isSteep) plot_pixel(y, x, line_color);
    else plot_pixel(x, y, line_color);

    error += dY;
    if(error >= 0) {
      y += yStep;
      error -= dX;
    }
  }
}


//draws a rectangle of width x height starting at top left
void draw_rectangle(int x, int y, int width, int height, short int color) {
  int init_x = x, init_y = y;

  for(x = init_x; x < init_x + width; x++) {
    for(y = init_y; y < init_y + height; y++) {
      //check boundaries
      if(x>=0 && y>=0 && x<SCREEN_X && y<SCREEN_Y) {
        plot_pixel(x, y, color);
      }
    }
  }
}


//plots a pixel
void plot_pixel(int x, int y, short int color) {
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = color;
}


//swap two ints
void swap(int* x, int* y) {
  int temp = *y;
  *y = *x;
  *x = temp;
}

void pushbutton_ISR( void )
{
	volatile int * KEY_ptr = (int *) KEY_BASE;
	volatile int * LED_ptr = (int *) LED_BASE;
	int press, LED_bits;

	press = *(KEY_ptr + 3);					// read the pushbutton interrupt register
	*(KEY_ptr + 3) = press;					// Clear the interrupt

	if (press & 0x1) {				// KEY0
		LED_bits = 0b1;
    player_dx = 1;
    player_dy = 0;
	} else if (press & 0x2)	{				// KEY1
		LED_bits = 0b10;
    player_dx = -1;
    player_dy = 0;
	} else if (press & 0x4) {
		LED_bits = 0b100;
    player_dy = -1;
    player_dx = 0;
	} else if (press & 0x8) {
		LED_bits = 0b1000;
    player_dy = 1;
    player_dx = 0;
  }

	*LED_ptr = LED_bits;
	return;
}
