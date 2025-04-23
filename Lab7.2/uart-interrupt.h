#ifndef UART_H_
#define UART_H_

#include <inc/tm4c123gh6pm.h>
#include <stdint.h>
#include <stdbool.h>
#include "driverlib/interrupt.h"

// Global variables for commands
extern volatile char command_byte_go;   // Command byte for 'g'
extern volatile int command_flag_go;     // Flag for 'g' command
extern volatile char command_byte_stop; // Command byte for 's'
extern volatile int command_flag_stop;   // Flag for 's' command
extern volatile int command_flag_manual; // Flag for 't' command
extern volatile char last_received_char; // Last received character
extern volatile bool is_manual_mode;     // Flag for manual mode

// UART1 device initialization for CyBot to PuTTY
void uart_interrupt_init(void);

// Send a byte over UART1 from CyBot to PuTTY
void uart_sendChar(char data);

// CyBot waits (i.e., blocks) to receive a byte from PuTTY
// returns byte that was received by UART1
// Not used with interrupts; see UART1_Handler
char uart_receive(void);

// Non-blocking UART receive function
// Returns 1 if data was available and read, 0 otherwise
int uart_receive_nonblocking(char *data);

// Send a string over UART1
// Sends each char in the string one at a time
void uart_sendStr(const char *data);

// Interrupt handler for receive interrupts
void UART1_Handler(void);

#endif /* UART_H_ */
