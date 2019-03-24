//function declarations for exceptions

/*
 * Configure registers in the GIC for an individual interrupt ID
 * We configure only the Interrupt Set Enable Registers (ICDISERn) and Interrupt
 * Processor Target Registers (ICDIPTRn). The default (reset) values are used for
 * other registers in the GIC
*/
void config_interrupt (int, int);

/*
 * Turn off interrupts in the ARM processor
*/
void disable_A9_interrupts (void);

/*
 * Initialize the banked stack pointer register for IRQ mode
*/
void set_A9_IRQ_stack (void);

/*
 * Configure the Generic Interrupt Controller (GIC)
*/
void config_GIC (void);

/* setup the KEY interrupts in the FPGA */
void config_KEYs (void);

/*
 * Turn on interrupts in the ARM processor
*/
void enable_A9_interrupts (void);
