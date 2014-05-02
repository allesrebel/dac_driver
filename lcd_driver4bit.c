/*
 * lcd_driver4bit.c
 * Library version of the 4bit LCD_driver written for
 * Microprocessor class.
 * 	   Version: 0.9
 *  Created on: May 1, 2014
 *     Authors: Rebel, Manrique
 */

#include "lcd_driver4bit.h"

//LCD write Instruction cycle function 4 bit
void lcd_wr_inst(char inst) {
	// Sets RS and RW low
	sendToPort(~(RW + RS), RW | RS, &CTLPout);

	// Set top 4bits to DB7-DB4
	sendToPort(HI_NIBBLE(inst), DB, &DBPout);
	sendEnable();

	// Set bottom 4bits to DB7-DB4
	sendToPort(LO_NIBBLE(inst), DB, &DBPout);
	sendEnable();

	__delay_cycles(800);
}

/*
 * Translation Functions -
 * Converts a command into databus compatiable signals
 * eg if 0x20 was given this function will take 2
 * 	and convert 0 0 1 0 -> D7 D6 D5 D4 with all other bits 0
 * 	regardless of where the db actually lies in terms of ports
 */
char HI_NIBBLE(char nib) {
	//grab the top nibble
	char preMap = ((nib) & 0xF0);
	//output starts at 0
	char result = 0;

	//now bit Checkin Time!
	//is bit& of preMap a 1?
	if(preMap & BIT7)
		result |= D7;	//add D7 to data out
	if(preMap & BIT6)
		result |= D6;
	if(preMap & BIT5)
		result |= D5;
	if(preMap & BIT4)
		result |= D4;

	//spit out the result
	return result;
}
//Take advantage of our earlier code
char LO_NIBBLE(char nib) {
	return HI_NIBBLE(nib << 4);
}

/*
 * Sends data to port safely without messing with other
 * pins on the port.
 * 	char data	- character containing the data to be written (note endian MSB -> LSB)
 * 	char pins	- which pins will be written to only (1 means write)
 * 	void* port	- the port the data will be written to
  * TODO: Parhaps make the inputs all port generic as well...
 */
void sendToPort(char data, char pins, volatile unsigned char* port) {
	// filter out values that don't have pins associated with them
	unsigned char orMask = data & pins;

	//Apply to port (only will set 1's won't set 0's of data)
	*port |= orMask;

	//alter mask for an AND operation
	unsigned char andMask = orMask | ~pins;

	//set the zero's where they should be
	*port &= andMask;
}

/*
 * Function_set
 * A special version of the function set command that
 * enables 4-bit mode. Follows the init procedure given by
 * the HD44780 Datasheet
 */
void function_set(char set) {
	// Sets RS and RW low (instr mode)
	sendToPort(~(RW + RS), RW | RS, &CTLPout);

	//send data through the databus
	sendToPort(0x30, DB, &DBPout);
	sendEnable();
	//wait for ~5ms according to datasheet
	__delay_cycles(106666);

	//send data through the databus
	sendToPort(0x30, DB, &DBPout);
	sendEnable();
	// Delay 100us from datasheet
	__delay_cycles(640);

	//send data through the databus
	sendToPort(0x30, DB, &DBPout);
	sendEnable();
	__delay_cycles(640);

	sendToPort(HI_NIBBLE(set), DB, &DBPout);
	sendEnable();
	__delay_cycles(640); 	// Delay 40us.

	sendToPort(HI_NIBBLE(set), DB, &DBPout);
	sendEnable();
	__delay_cycles(640); 	// Delay 40us.

	sendToPort(LO_NIBBLE(set), DB, &DBPout);
	sendEnable();
	__delay_cycles(640); 	// Delay 40us.
}

/*
 * Function to clock the data to the LCD
 * Timing should be adjusted if using a clock other than 16Mhz
 */
void sendEnable() {
	CTLPout |= E;  // sets enable high
	__delay_cycles(4); // delay 250ns
	CTLPout &= (~E);  //Enable low
}

/*
 * Initialize LCD
 * Initializes LCD and sets up with the ports defined!
 * Does everything that's needed to get the LCD driven
 * in 4-bit mode
 * TODO: Add in the ability to turn off various settings. eg. cursors
 */
void lcd_initialize(void) {
	// 30ms needed to initialize LED
	__delay_cycles(640000); // Need 30ms start-up delay time. Delays 40ms.

	//Make sure the control lines are low before they become outputs
	sendToPort(~(E + RS + RW), E | RS | RW, &CTLPout);
	sendToPort(E | RS | RW, E | RS | RW, &CTLPort);	//now they are outputs

	//set data lines to output
	sendToPort(DB, DB, &DBPort);

	//Implement Function set Instruction: 2-line mode with display on
	function_set(0x2C);
	__delay_cycles(640); 	// Delay 40us.

	//Implement Display control Instruction: display on, cursor on, and blink on.
	lcd_wr_inst(0x0F);
	__delay_cycles(640); 	// Delay 40us.

	//Implement Clear screen instruction 0x01
	lcd_wr_inst(0x01);
	__delay_cycles(32000); 	// Delay 2ms.

	//Implement Return Home instruction 0x02
	lcd_wr_inst(0x02);
	__delay_cycles(32000);	// Delay 2ms.
}

// LCD data write function
void lcd_wr_data(char data) {
	//RW low with RS high for data write mode
	sendToPort(~RW | RS, RW | RS, &CTLPout);

	// Set top 4bits to DB7-DB4
	sendToPort(HI_NIBBLE(data), DB, &DBPout);
	sendEnable();

	// Set bottom 4bits to DB7-DB4
	sendToPort(LO_NIBBLE(data), DB, &DBPout);
	sendEnable();

	__delay_cycles(800);
}

// Send String
void lcd_send_string(char *str) {
	while (*str) {
		lcd_wr_data(*str);	//write data
		str++;
	}
}

//Line at location 00 0000 in CGRAM (Pattern 0)
void make_line() {
	lcd_wr_inst(0x40);
	lcd_wr_data(0x04);

	lcd_wr_inst(0x41);
	lcd_wr_data(0x04);

	lcd_wr_inst(0x42);
	lcd_wr_data(0x04);

	lcd_wr_inst(0x43);
	lcd_wr_data(0x04);

	lcd_wr_inst(0x44);
	lcd_wr_data(0x04);

	lcd_wr_inst(0x45);
	lcd_wr_data(0x04);

	lcd_wr_inst(0x46);
	lcd_wr_data(0x04);

	lcd_wr_inst(0x47);
	lcd_wr_data(0x04);
}

