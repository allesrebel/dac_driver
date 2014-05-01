#include <msp430.h> 
//#include "lcd_driver4bit.h"

void Drive_DAC(unsigned int level);
void setParameters(int freq, int samples, float duty);
void initTimer();
/*
 * Pin config - to make it easier to change if needed
 */
#define CS		BIT4
#define CSPOut	P1OUT
#define CSPort	P1DIR

/*
 * Constants and Variables
 */
volatile unsigned int freqs[5] = { 100, 200, 300, 400, 500 };
volatile unsigned int count = 0;
volatile unsigned int state = 0;
volatile unsigned int sin_pos = 0;
volatile unsigned int current_freq = 0;
volatile unsigned int sqaure_val = 0;
volatile unsigned int TempDAC_Value = 0;
volatile unsigned int sampledCycles[5] = { 1600, 800, 533, 400, 320 };
volatile unsigned int SIN_Values[50] = { 862, 703, 553, 415, 292, 188, 105, 45,
		10, 0, 17, 59, 125, 214, 324, 451, 593, 746, 906, 1069, 1230, 1387,
		1534, 1669, 1787, 1885, 1962, 2015, 2044, 2046, 2022, 1973, 1901, 1805,
		1691, 1559, 1414, 1259, 1097, 935 }; // Sin values rounded to nearest int.

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

	// 16Mhz SMCLK
	if (CALBC1_16MHZ == 0xFF)	// if calibration constant erased
			{
		while (1)
			;			// Do not load trap cpu
	}
	DCOCTL = 0;
	BCSCTL1 = CALBC1_16MHZ;	// Set range
	DCOCTL = CALDCO_16MHZ;	// Set DCO step + modulation

	//Initialize Sin values
	P1DIR &= ~BIT3;
	P1IE |= BIT3;                   // interrupts enabled on port 3
	P1IES |= BIT3;
	P1IFG &= ~(BIT3);

	//Initialize Ports
	CSPort |= CS;		//Use bit to activate CE (active low) on the DAC(pin 2)

	P1SEL = BIT7 + BIT5;	// These two lines select UCB0CLK for P1.5
	P1SEL2 = BIT7 + BIT5;	// and select UCB0SIMO for P1.7

	UCB0CTL0 |= UCCKPL + UCMSB + UCMST + UCSYNC;

	UCB0CTL1 |= UCSSEL_2; 	// sets UCB0 will use SMCLK as the bit shift clock

	UCB0CTL1 &= ~UCSWRST;	//SPI enable

	initTimer();
	//set up timer with 100Hz initially
	setParameters(freqs[current_freq], 100, 50);

	_BIS_SR(LPM0_bits + GIE);

	while (1) {
		setParameters(freqs[current_freq], 100, 50);
	}
}

void Drive_DAC(unsigned int level) {
	unsigned int DAC_Word = 0;

	DAC_Word = (0x1000) | (level & 0x0FFF); // 0x1000 sets DAC for Write

	CSPOut &= ~CS; //Select the DAC

	UCB0TXBUF = (DAC_Word >> 8); // Shift upper byte of DAC_Word
	// 8-bits to right

	while (!(IFG2 & UCB0TXIFG))
		; // USCI_A0 TX buffer ready?

	UCB0TXBUF = (unsigned char) (DAC_Word & 0x00FF); // Transmit lower byte to DAC

	while (!(IFG2 & UCB0TXIFG))
		; // USCI_A0 TX buffer ready?

	CSPOut |= CS; //Unselect the DAC
	return;
}

/*
 * Set up the timer TA0
 */
void initTimer() {
	TA0CCTL0 = CCIE;			// Enable interrupt
	TA0CTL = TASSEL_2 + MC_1;	//SMCLK src, with count up mode
}

/*
 * Set the Timer TA0's Frequency
 * 	param freq is the target frequency (effects CCR0)
 * 	param samples is the sample count (defaults 100 right now) TODO: probably should put more
 * 	param duty sets the duty cycle stuff (effects CCR1)
 */
void setParameters(int freq, int samples, float duty) {
	switch (freq) {
	case 500:
		TA0CCR0 = sampledCycles[4];
		break;
	case 400:
		TA0CCR0 = sampledCycles[3];
		break;
	case 300:
		TA0CCR0 = sampledCycles[2];
		break;
	case 200:
		TA0CCR0 = sampledCycles[1];
		break;
	case 100:
		TA0CCR0 = sampledCycles[0];
		break;
	}
}

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A(void) {

	switch (state) {
	case 0:
		//This is a sqaure wave state
		if (count % 50 == 0) {
			if (sqaure_val == 0)
				sqaure_val = 2048;
			else
				sqaure_val = 0;
		}
		count++;

		//drive dac somehow
		Drive_DAC(sqaure_val);
		break;
	case 1:
		//sine wave
		SIN_Values[sin_pos];
		switch (freq) {
		case 500:

			break;
		case 400:
			sin_pos++;
			break;
		case 300:
			sin_pos++;
			break;
		case 200:
			sin_pos++;
			break;
		case 100:
			sin_pos++;
			break;
		}
		sin_pos++;

		break;
	case 2:
		//sawtooth

		break;
	}
}

//Button push interrupt ISR
#pragma vector=PORT1_VECTOR
__interrupt void ISR_PORT1(void) {
	if ((P1IFG & BIT3)== BIT3) {
		if (current_freq + 1 > 4) {
			current_freq = -1;
		}
		current_freq++;

		setParameters(freqs[current_freq], 100, 50); //set it
		P1IFG &= ~BIT3;	//clear interrupt flag
	}
}
