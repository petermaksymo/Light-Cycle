#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "address_map_arm.h"
#include "interrupt_ID.h"
#include "defines.h"
#include "exceptions.h"
#include "main.h"

volatile int pixel_buffer_start; // global variable

volatile char byte1, byte2, byte3; // PS/2 variables

volatile int timeout; // used to synchronize with the timer

volatile int* switches_ptr = SW_BASE;

Player_State init_player_1 = {
  BOARD_X/8, BOARD_Y/8, 1, 0, 1, 0x07E0, true
};

Player_State init_player_2 = {
  BOARD_X - BOARD_X/8,  BOARD_Y - BOARD_Y/8, -1, 0, 2, 0xF800, true
};

Player_State init_player_3 = {
   BOARD_X/8, BOARD_Y - BOARD_Y/8, 1, 0, 3, 0x001F, true
};

Player_State init_player_4 = {
  BOARD_X - BOARD_X/8, BOARD_Y/8, -1, 0, 4, 0xF81F, true
};

Player_State* players_ptr;

volatile bool game_start = false;

int main(void) {
    set_A9_IRQ_stack ();			// initialize the stack pointer for IRQ mode
    config_GIC ();					// configure the general interrupt controller
    config_PS2();  // configure PS/2 port to generate interrupts
    //config_KEYs ();				// configure pushbutton KEYs to generate interrupts

    enable_A9_interrupts ();	// enable interrupts in the A9 processor

    volatile int * pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;

    //initialize the game board, represents the screen. Each pixel here will
    //contain the player number or 0 for none
    int game_board[BOARD_X][BOARD_Y] = { 0 };

    //initialize player start positions
    Player_State players[4];

    players[0] = init_player_1;
    players[1] = init_player_2;
    players[2] = init_player_3;
    players[3] = init_player_4;
    players_ptr = players;

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

    while(!game_start) {
      int i;
      for(i = 0; i < NUM_PLAYERS; i++) {
        //set number of players (can be dynamic for shenanigans)
        if((*switches_ptr & 0x300) >> 8 < i) {
          players[i].is_alive = false;
          game_board[ players[i].pos_x ][ players[i].pos_y ] = 0;
        } else {
          players[i].is_alive = true;
          game_board[ players[i].pos_x ][ players[i].pos_y ] = players[i].value;
        }
      }

      draw_board(game_board, players);
      wait_for_vsync(); // swap front and back buffers on VGA vertical sync
      pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }

    int speed_counter = 0;
    int score = 0, score_counter = 0;
    bool game_done = false;
    while (1) {
      int speed_limit = *switches_ptr & 0b1111;
      speed_counter++;
      int i;
      //use speed_counter and speed_limit to adjust the speed of players, higher is slower
      if(speed_counter > speed_limit) {
        speed_counter = 0;
        game_done = true;
        for(i = 0; i < NUM_PLAYERS; i++) {
          if(players[i].is_alive) {
            game_done = false;
            game_board[ players[i].pos_x ][ players[i].pos_y ] = players[i].value;
            players[i].pos_x += players[i].dx;
            players[i].pos_y += players[i].dy;

            if(game_board[players[i].pos_x][players[i].pos_y] != 0 ||
                players[i].pos_x > BOARD_X || players[i].pos_y > BOARD_Y ||
                players[i].pos_x < 0 || players[i].pos_y < 0
            ) {
                kill_player(players[i].value, game_board);
                players[i].is_alive = false;
            }
          }
        }
      }

      draw_board(game_board, players);

      update_score(score);
      score_counter ++;
      if(score_counter == 15 && !game_done) {
        score ++;
        score_counter = 0;
      }

      wait_for_vsync(); // swap front and back buffers on VGA vertical sync
      pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

    }
}

/* setup the PS/2 interrupts */
void config_PS2() {
    volatile int * PS2_ptr = (int *)PS2_BASE; // PS/2 port address

    *(PS2_ptr) = 0xFF; /* reset */
    *(PS2_ptr + 1) =
        0x1; /* write to the PS/2 Control register to enable interrupts */
}

/****************************************************************************************
 * Subroutine to show a string of HEX data on the HEX displays
****************************************************************************************/
void update_score(int score) {
    volatile int * HEX3_HEX0_ptr = (int *)HEX3_HEX0_BASE;
    volatile int * HEX5_HEX4_ptr = (int *)HEX5_HEX4_BASE;

    /* SEVEN_SEGMENT_DECODE_TABLE gives the on/off settings for all segments in
     * a single 7-seg display in the DE2 Media Computer, for the hex digits 0 -
     * F */
    unsigned char seven_seg_decode_table[] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
        0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};
    unsigned char hex_segs[] = {0, 0, 0, 0, 0, 0, 0, 0};
    unsigned int  shift_buffer, nibble;
    unsigned char code;
    int           i;

    shift_buffer = (score%10) | ((score/10)%10 << 4) | ((score/100)%10 << 8) | ((score/1000)%10 << 12);
    for (i = 0; i < 6; ++i) {
        nibble = shift_buffer & 0x0000000F; // character is in rightmost nibble
        code   = seven_seg_decode_table[nibble];
        hex_segs[i]  = code;
        shift_buffer = shift_buffer >> 4;
    }
    /* drive the hex displays */
    *(HEX3_HEX0_ptr) = *(int *)(hex_segs);
    *(HEX5_HEX4_ptr) = *(int *)(hex_segs + 4);
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
void draw_board(int game_board[BOARD_X][BOARD_Y], Player_State players[4]) {
  int board_to_screen_factor = SCREEN_X / BOARD_X;

  int x, y, x_screen, y_screen;
  for(x = 0; x < BOARD_X; x++) {
    for(y = 0; y < BOARD_Y; y++) {
      x_screen = x * board_to_screen_factor;
      y_screen = y * board_to_screen_factor;

      switch(game_board[x][y]) {
        case 0: draw_rectangle(x_screen, y_screen, board_to_screen_factor, board_to_screen_factor, 0x0000); break;
        case 1: draw_rectangle(x_screen, y_screen, board_to_screen_factor, board_to_screen_factor, players[0].color); break;
        case 2: draw_rectangle(x_screen, y_screen, board_to_screen_factor, board_to_screen_factor, players[1].color); break;
        case 3: draw_rectangle(x_screen, y_screen, board_to_screen_factor, board_to_screen_factor, players[2].color); break;
        case 4: draw_rectangle(x_screen, y_screen, board_to_screen_factor, board_to_screen_factor, players[3].color); break;

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

void kill_player(int player, int game_board[BOARD_X][BOARD_Y]) {
  int x, y;

  for(x = 0; x < BOARD_X; x++) {
    for(y = 0; y < BOARD_Y; y++){
      if(game_board[x][y] == player)
        game_board[x][y] = 0;
    }
  }
}
