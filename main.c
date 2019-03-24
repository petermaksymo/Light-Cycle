#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define SCREEN_X 320
#define SCREEN_Y 240

#define TOTAL_RECTANGLES 8

volatile int pixel_buffer_start; // global variable

void clear_screen();
void wait_for_vsync();
void draw_line(int x0, int y0, int  x1, int y1, short int line_color);
void draw_rectangle(int x, int y, int width, int height, short int color);
void plot_pixel(int x, int y, short int line_color);
void swap(int* x, int* y);

int main(void) {
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;

    // initialize location and direction of rectangles
    int rects_x[TOTAL_RECTANGLES], rects_y[TOTAL_RECTANGLES],
        rects_dx[TOTAL_RECTANGLES], rects_dy[TOTAL_RECTANGLES],
        rects_color[TOTAL_RECTANGLES];

    //randomize the rectangle initial position/direction
    for(int i = 0; i < TOTAL_RECTANGLES; i++) {
      rects_x[i] = rand()%SCREEN_X;
      rects_y[i] = rand()&SCREEN_Y;
      rects_dx[i] = (rand()%2)*2-1;
      rects_dy[i] = (rand()%2)*2-1;
      rects_color[i] = rand()%(0xFFFF);
    }

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the
                                        // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer

    while (1)
    {
        clear_screen();

        //draw lines between rectangles
        for(int i = 0; i < TOTAL_RECTANGLES - 1; i++) {
          draw_rectangle(rects_x[i], rects_y[i], 2, 2, rects_color[i]);
          draw_line(rects_x[i], rects_y[i], rects_x[i+1], rects_y[i+1], rects_color[i]);
        }
        draw_rectangle(rects_x[TOTAL_RECTANGLES-1], rects_y[TOTAL_RECTANGLES-1], 2, 2, rects_color[TOTAL_RECTANGLES-1]);
        draw_line(rects_x[TOTAL_RECTANGLES-1], rects_y[TOTAL_RECTANGLES-1], rects_x[0], rects_y[0], rects_color[TOTAL_RECTANGLES-1]);

        //movement and adjust direction to bounce of edges of screen
        for(int i = 0; i < TOTAL_RECTANGLES; i++) {
          rects_x[i] += rects_dx[i];
          rects_y[i] += rects_dy[i];


          if(rects_x[i] <= 0 || rects_x[i] >= SCREEN_X) rects_dx[i] = rects_dx[i]*-1;
          if(rects_y[i] <= 0 || rects_y[i] >= SCREEN_Y) rects_dy[i] = rects_dy[i]*-1;
        }

        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
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
  volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
  register int status;

  *pixel_ctrl_ptr = 1; // start the synchronization process

  status = *(pixel_ctrl_ptr + 3);
  while ((status & 0x01) != 0) {
    status = *(pixel_ctrl_ptr + 3);
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


//draws a rectangle, centered around x and y
void draw_rectangle(int x, int y, int half_width, int half_height, short int color) {
  int init_x = x, init_y = y;
  int final_x = x + half_width;
  int final_y = y + half_height;

  for(x = init_x - half_width; x <= final_x; x++) {
    for(y = init_y - half_height; y <= final_y; y++) {
      //check boundaries
      if(x>=0 && y>=0 && x<SCREEN_X && y<SCREEN_Y) {
        plot_pixel(x, y, color);
      }
    }
  }
}


//plots a pixel
void plot_pixel(int x, int y, short int line_color) {
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}


//swap two ints
void swap(int* x, int* y) {
  int temp = *y;
  *y = *x;
  *x = temp;
}
