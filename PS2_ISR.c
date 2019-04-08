#include "main.h"

extern volatile char byte1, byte2, byte3;
//extern int player_dx;
//extern int player_dy;

extern Player_State* players_ptr;

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
	volatile int * LED_ptr = (int *) 0xFF200000;
	int PS2_data, RAVAIL, LED_bits;

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
	if (byte3 == 0x23) {				// D
    LED_bits = 0b1;
    players_ptr[0].dx = players_ptr[0].dx == -1 ? -1 : 1;
    players_ptr[0].dy = 0;
	} else if (byte3 == 0x1C)	{				// A
    LED_bits = 0b10;
    players_ptr[0].dx = players_ptr[0].dx == 1 ? 1 : -1;
    players_ptr[0].dy = 0;
	} else if (byte3 == 0x1D) {           // W
    LED_bits = 0b100;
    players_ptr[0].dy = players_ptr[0].dy == 1 ? 1 : -1;
    players_ptr[0].dx = 0;
	} else if (byte3 == 0x1B) {          // D
    LED_bits = 0b1000;
    players_ptr[0].dy = players_ptr[0].dy == -1 ? -1 : 1;
    players_ptr[0].dx = 0;
  }
  *LED_ptr = LED_bits;
	return;
}
