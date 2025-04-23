///*
//*
//*   uart.c
//*
//*
//*
//*   @author Embe DidSystems
//*   @date
//*/
//
//#include <inc/tm4c123gh6pm.h>
//#include <stdint.h>
//#include "uart.h"
//
//void uart_init(void){
//	//TODO
//  //enable clock to GPIO port B
//  SYSCTL_RCGCGPIO_R |= 0x0002;
//
//  //enable clock to UART1
//  SYSCTL_RCGCUART_R |= 0x0002;
//
//  //wait for GPIOB and UART1 peripherals to be ready
//  while ((SYSCTL_PRGPIO_R & 0x0002) == 0) {};
//  while ((SYSCTL_PRUART_R & 0x0002) == 0) {};
//
//  //enable alternate functions on port B pins
//  GPIO_PORTB_AFSEL_R |= 0x0003;
//
//  //enable digital functionality on port B pins
//  GPIO_PORTB_DEN_R |= 0x0003;
//
//  //enable UART1 Rx and Tx on port B pins
//  GPIO_PORTB_PCTL_R &= 0xFFFFFF00;  //xxxx yyyy ==> 0000 0000 // &= !0xFF ==> FFFFFF00
//  GPIO_PORTB_PCTL_R |= 0x00000011; //
////  ==> 0001 0001
//
//  //calculate baud rate
//  uint16_t iBRD = 8; //use equations
//  uint16_t fBRD = 44; //use equations
//
//  //turn off UART1 while setting it up
//  UART1_CTL_R &= ~0x0001;
//
//  //set baud rate
//  //note: to take effect, there must be a write to LCRH after these assignments
//  UART1_IBRD_R = iBRD;
//  UART1_FBRD_R = fBRD;
//
//  //set frame, 8 data bits, 1 stop bit, no parity, no FIFO
//  //note: this write to LCRH must be after the BRD assignments
//  UART1_LCRH_R = 0x0060;
//
//  //use system clock as source
//  //note from the datasheet UARTCCC register description:
//  //field is 0 (system clock) by default on reset
//  //Good to be explicit in your code
//  UART1_CC_R = 0x0;
//
//  //re-enable UART1 and also enable RX, TX (three bits)
//  //note from the datasheet UARTCTL register description:
//  //RX and TX are enabled by default on reset
//  //Good to be explicit in your code
//  //Be careful to not clear RX and TX enable bits
//  //(either preserve if already set or set them)
//  UART1_CTL_R = 0x0301;
//
//}
//
//void uart_sendChar(char data){
//	//TODO
//    while(UART1_FR_R & 0x20) {
//    }
//    UART1_DR_R = data;
//}
//
//char uart_receive(void){
//	//TODO
//    char receiver = 0;
//
//    // This comment tells you stuff
//    while(UART1_FR_R & UART_FR_RXFE) {};
//
//    receiver = (char) (UART1_DR_R & 0xFF);
//    return receiver;
//
//}
//
//void uart_sendStr(const char *data){
//	//TODO for reference see lcd_puts from lcd.c file
////    for (i = 0; i < strlen(data); i++) {
//    while(*data) {
//        uart_sendChar(*data);
//        data++;
//    }
//}
