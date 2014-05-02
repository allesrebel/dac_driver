# MCP4901 Driver for MSP430G2553
Warning: Not a bit-banged version! This code uses the USCI hardware built into the G2553 to drive the DAC via SPI. 

Pretty basic function generator code, generates a sqaure wave with 5 frequencies, sine, and a sawtooth.

#DAC Driver Specs
Takes roughly 1.5us per call of drive_dac(), can't get any faster on 16Mhz clock

#Function Generator Specs
1. Shall have 3 waveforms
  * Sqaure 
  * Sine 
  * Sawtooth
2. All waveforms have 5 adjustable frequencies
  * 100, 200, 300, 400, 500
3. Sqaurewave is allowed to vary in duty cycle by 10% on button push

## Written by Alles Rebel and Evan Manrique
Written for a microprocessor based design course at Calpoly San Luis Obispo

## Changelog 
 * 5/1/2014 .99
 	* Added some lazy ways of converting ints to strings
 	* Some minor tweaks - cleaned up LCD library
 	* fixed some more minor bugs and comments
 * 5/1/2014 0.9
 	* Added in updated LCD library
 	* Added LCD into demo
