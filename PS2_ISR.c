#include "main.h"

extern volatile char byte1, byte2, byte3;
//extern int player_dx;
//extern int player_dy;

extern Player_State* players_ptr;
extern bool game_start;

/***************************************************************************************
 * Pushbutton - Interrupt Service Routine
 *
 * This routine checks which KEY has been pressed. If it is KEY1 or KEY2, it writes this
 * value to the global variable key_pressed. If it is KEY3 then it loads the SW switch
 * values and stores in the variable pattern
****************************************************************************************/
void PS2_ISR( void )
{
	volatile int * PS2_ptr = (int *) 0xFF200100;		// PS/2 port address
	int PS2_data, RAVAIL;

	PS2_data = *(PS2_ptr);									// read the Data register in the PS/2 port
	RAVAIL = (PS2_data & 0xFFFF0000) >> 16;			// extract the RAVAIL field
	if (RAVAIL > 0)
	{
		/* always save the last three bytes received */
		byte1 = byte2;
		byte2 = byte3;
		byte3 = PS2_data & 0xFF;
		if ( (byte2 == (char) 0xAA) && (byte3 == (char) 0x00) )
			// mouse inserted; initialize sending of data
			*(PS2_ptr) = 0xF4;
	}

  if(byte3 == 0x29) {
    game_start = true;
    return;
  }

  if(byte2 == 0xF0) return; //throw out breaks for game consistency

  //Player 0 WASD
	if (byte3 == 0x23) {				// D
    players_ptr[0].dx = players_ptr[0].dx == -1 ? -1 : 1;
    players_ptr[0].dy = 0;
	} else if (byte3 == 0x1C)	{				// A
    players_ptr[0].dx = players_ptr[0].dx == 1 ? 1 : -1;
    players_ptr[0].dy = 0;
	} else if (byte3 == 0x1D) {           // W
    players_ptr[0].dy = players_ptr[0].dy == 1 ? 1 : -1;
    players_ptr[0].dx = 0;
	} else if (byte3 == 0x1B) {          // S
    players_ptr[0].dy = players_ptr[0].dy == -1 ? -1 : 1;
    players_ptr[0].dx = 0;
  }

//player 1 ARROW KEYS
  if ((byte1 == 0xE0 || byte2 == 0xE0) && byte3 == 0x74) {				// right arrow
    players_ptr[1].dx = players_ptr[1].dx == -1 ? -1 : 1;
    players_ptr[1].dy = 0;
	} else if ((byte1 == 0xE0 || byte2 == 0xE0) && byte3 == 0x6B)	{				// left arrow
    players_ptr[1].dx = players_ptr[1].dx == 1 ? 1 : -1;
    players_ptr[1].dy = 0;
	} else if ((byte1 == 0xE0 || byte2 == 0xE0) && byte3 == 0x75) {           // up arrow
    players_ptr[1].dy = players_ptr[1].dy == 1 ? 1 : -1;
    players_ptr[1].dx = 0;
	} else if ((byte1 == 0xE0 || byte2 == 0xE0) && byte3 == 0x72) {          // down arrow
    players_ptr[1].dy = players_ptr[1].dy == -1 ? -1 : 1;
    players_ptr[1].dx = 0;
  }

  //Player 2 IJKL
  if (byte3 == 0x4B) {				// L
    players_ptr[2].dx = players_ptr[2].dx == -1 ? -1 : 1;
    players_ptr[2].dy = 0;
	} else if (byte3 == 0x3B)	{				// J
    players_ptr[2].dx = players_ptr[2].dx == 1 ? 1 : -1;
    players_ptr[2].dy = 0;
	} else if (byte3 == 0x43) {           // I
    players_ptr[2].dy = players_ptr[2].dy == 1 ? 1 : -1;
    players_ptr[2].dx = 0;
	} else if (byte3 == 0x42) {          // K
    players_ptr[2].dy = players_ptr[2].dy == -1 ? -1 : 1;
    players_ptr[2].dx = 0;
  }

  //Player 3 NUM PAD
  if ((byte1 != 0xE0 && byte2 != 0xE0) && byte3 == 0x74) {				// numpad 6
    players_ptr[3].dx = players_ptr[3].dx == -1 ? -1 : 1;
    players_ptr[3].dy = 0;
	} else if ((byte1 != 0xE0 && byte2 != 0xE0) && byte3 == 0x6B)	{				// num pad 4
    players_ptr[3].dx = players_ptr[3].dx == 1 ? 1 : -1;
    players_ptr[3].dy = 0;
	} else if ((byte1 != 0xE0 && byte2 != 0xE0) && byte3 == 0x75) {           // num pad 8
    players_ptr[3].dy = players_ptr[3].dy == 1 ? 1 : -1;
    players_ptr[3].dx = 0;
	} else if ((byte1 != 0xE0 && byte2 != 0xE0) && byte3 == 0x73) {          // num pad 5
    players_ptr[3].dy = players_ptr[3].dy == -1 ? -1 : 1;
    players_ptr[3].dx = 0;
  }

	return;
}
