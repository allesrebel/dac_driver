/*
 * lcd_driver4bit.h
 * Library version of the 4bit LCD_driver written for
 * Microprocessor class.
 *  Created on: May 1, 2014
 *     Authors: Rebel, Manrique
 */

#ifndef LCD_DRIVER4BIT_H_
#define LCD_DRIVER4BIT_H_

//needs this for delay_cycles and port definitions
#include <msp430.h>

/*
 * Define control bits
 */
#define E		BIT2
#define RW		BIT1
#define RS		BIT0
#define CTLPout	P1OUT	//output port for the ctrl sigs
#define CTLPort	P1DIR	//port information
/*
 * Define Data bus bits TODO: Make these work for any data lines (not just 7->4)
 */
#define D7 		BIT7
#define D6 		BIT6
#define D5 		BIT5
#define D4 		BIT4
#define DBPout	P1OUT	//output port for the databus
#define DBPin	P1IN	//input port for the databus
#define DBPort	P1DIR	//port information
#define DB D7|D6|D5|D4	//useful alias
/*
 * Functions
 */
//Support Functions (internals rely on these)
void sendToPort(char data, char pins, volatile unsigned char* port);
void sendEnable();
char HI_NIBBLE(char nib);
char LO_NIBBLE(char nib);
void function_set(char set);

//Initialization
void lcd_initialize(void);

//External Functions
void lcd_wr_inst(char inst);
void lcd_wr_data(char data);
void lcd_send_string(char* str);
void lcd_move_cursor(void);
void lcd_add_space(int num);

//incomplete functions
//Custom Characters Test Implementation
void make_line();

#endif /* LCD_DRIVER4BIT_H_ */
