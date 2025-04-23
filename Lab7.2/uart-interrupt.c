#include "uart-interrupt.h"

// Global variables for commands
volatile char command_byte_go = 'g';    // Command byte for 'g'
volatile int command_flag_go = 0;       // Flag for 'g' command
volatile char command_byte_stop = 's';  // Command byte for 's'
volatile int command_flag_stop = 0;     // Flag for 's' command
volatile int command_flag_manual = 0;   // Flag for 't' command
volatile char last_received_char = 0;   // Last received character
volatile bool is_manual_mode = false;   // Flag for manual mode

void uart_interrupt_init(void) {
    // Enable clock to GPIO port B
    SYSCTL_RCGCGPIO_R |= 0b000010;

    // Enable clock to UART1
    SYSCTL_RCGCUART_R |= 0x02;

    // Wait for GPIOB and UART1 peripherals to be ready
    while ((SYSCTL_PRGPIO_R & 0x02) == 0) {};
    while ((SYSCTL_PRUART_R & 0x02) == 0) {};

    // Enable digital functionality on port B pins
    GPIO_PORTB_DEN_R |= 0x03;

    // Enable alternate functions on port B pins
    GPIO_PORTB_AFSEL_R |= 0x03;

    // Enable UART1 Rx and Tx on port B pins
    GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & 0xFFFFFF00) + 0x00000011;

    // Calculate baud rate
    uint16_t iBRD = (int)(16000000 / (16 * 115200)); // Integer part of baud rate divisor
    uint16_t fBRD = (int)(0.6808 * 64 + 0.5);        // Fractional part of baud rate divisor

    // Turn off UART1 while setting it up
    UART1_CTL_R &= ~UART_CTL_UARTEN;

    // Set baud rate
    UART1_IBRD_R = iBRD;
    UART1_FBRD_R = fBRD;

    // Set frame: 8 data bits, 1 stop bit, no parity, no FIFO
    UART1_LCRH_R = UART_LCRH_WLEN_8;

    // Use system clock as source
    UART1_CC_R = UART_CC_CS_SYSCLK;

    // Clear RX interrupt flag (clear by writing 1 to ICR)
    UART1_ICR_R |= UART_ICR_RXIC;

    // Enable RX raw interrupts in interrupt mask register
    UART1_IM_R |= UART_IM_RXIM;

    // NVIC setup: set priority of UART1 interrupt to 1
    NVIC_PRI1_R = (NVIC_PRI1_R & 0xFF0FFFFF) | 0x00200000;

    // NVIC setup: enable interrupt for UART1, IRQ #6, set bit 6
    NVIC_EN0_R |= (1 << 6);

    // Tell CPU to use ISR handler for UART1
    IntRegister(INT_UART1, UART1_Handler);

    // Globally allow CPU to service interrupts
    IntMasterEnable();

    // Re-enable UART1 and also enable RX, TX (three bits)
    UART1_CTL_R |= (UART_CTL_UARTEN | UART_CTL_RXE | UART_CTL_TXE);
}

void uart_sendChar(char data) {
    while (UART1_FR_R & UART_FR_TXFF); // Wait until TX buffer is not full
    UART1_DR_R = data;
}

char uart_receive(void) {
    while (UART1_FR_R & UART_FR_RXFE); // Wait until RX buffer is not empty
    return (char)(UART1_DR_R & 0xFF);
}

int uart_receive_nonblocking(char *data) {
    if (UART1_FR_R & UART_FR_RXFE) {
        return 0; // Receive FIFO empty, no data available
    } else {
        *data = (char)(UART1_DR_R & 0xFF);
        return 1; // Data is available
    }
}

void uart_sendStr(const char *data) {
    while (*data != '\0') {
        uart_sendChar(*data);
        data++;
    }
}

void UART1_Handler(void) {
    char byte_received;

    if ((UART1_MIS_R & UART_MIS_RXMIS) == UART_MIS_RXMIS) {
        UART1_ICR_R |= UART_ICR_RXIC; // Clear RX trigger flag

        byte_received = (char)(UART1_DR_R & 0xFF);
        last_received_char = byte_received; // Store received character
        uart_sendChar(byte_received); // Echo back to PuTTY

        if (byte_received == 'g') {
            command_flag_go = 1;
        }
        if (byte_received == 's') {
            command_flag_stop = 1;
        }
        if (byte_received == 't') {
            is_manual_mode = !is_manual_mode; // Toggle manual mode
            command_flag_manual = 1; // Set the flag to indicate manual mode change
            uart_sendStr("\r\nManual mode ");
            uart_sendStr(is_manual_mode ? "enabled\r\n" : "disabled\r\n");
        }

    }
}


