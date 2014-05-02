#include <msp430.h> 
//#include "lcd_driver4bit.h"

void Drive_DAC(unsigned int level);
void setParameters(int freq, int sample);
void initTimer();

/*
 * Support functions for the duty cycle change
 */
void toggleSqaure();
void onSqaure();
void offSqaure();

//Debugging functions
void incrementFrequency();
void incrementState();
void incrementDuty();

/*
 * Pin configs - to make it easier to change if needed
 */
#define CS		BIT4
#define CSPOut	P1OUT
#define CSPort	P1DIR

//USCI General Port Info
#define USCIPS1	P1SEL	//USCI Pin mode select
#define USCIPS2	P1SEL2	//USCI Pin mode select 2
//UCA0 PORT Defs		-------NOT USED UCB is in USE -------
#define UCASOMI	BIT1	//isn't really used to communicate with DAC
#define UCASIMO	BIT2
#define UCACLK	BIT4

//UCB0 PORT Defs
#define UCBSOMI	BIT6	//isn't really used to communicate with DAC
#define UCBSIMO	BIT7
#define UCBCLK	BIT5

/*
 * Button config
 */
#define B0	BIT3
#define B1	BIT2
#define B2	BIT1

/*
 * Constants and Variables
 */
volatile unsigned int freqs[5] = { 100, 200, 300, 400, 500 };
volatile unsigned int count = 0;
volatile unsigned int state = 0;
volatile unsigned int sin_pos = 0;
volatile unsigned int current_freq = 0;
volatile unsigned int sqaure_val = 0;
volatile unsigned int saw_val = 0;
volatile unsigned int samples = 0;
volatile unsigned int duty = 50;
/*
 * Calibration data for cycle counts of various frequencies
 */
//Data for 100 samples for the whole period
volatile unsigned int cali100Samples[5] = { 1600, 800, 533, 400, 320 };

//First half of sine wave (only 50 samples for first half of period)
volatile unsigned int sine1_50Samples[50] = { 1024, 1088, 1152, 1215, 1278,
		1340, 1400, 1459, 1517, 1572, 1625, 1676, 1724, 1770, 1813, 1852, 1888,
		1921, 1950, 1976, 1997, 2015, 2029, 2039, 2045, 2048, 2045, 2039, 2029,
		2015, 1997, 1976, 1950, 1921, 1888, 1852, 1813, 1770, 1724, 1676, 1625,
		1572, 1517, 1459, 1400, 1340, 1278, 1215, 1152, 1088 };
//second half of sine wave (only 50 samples for second half of period)
volatile unsigned int sine2_50Samples[50] = { 1024, 959, 895, 832, 769, 707,
		647, 588, 530, 475, 422, 371, 323, 277, 234, 195, 159, 126, 97, 71, 50,
		32, 18, 8, 2, 0, 2, 8, 18, 32, 50, 71, 97, 126, 159, 195, 234, 277, 323,
		371, 422, 475, 530, 588, 647, 707, 769, 832, 895, 959 };

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

	//Initialize Button Stuff
	P1DIR &= ~(B0 + B1 + B2);	// set buttons as output
	P1IE |= (B0 + B1 + B2);		// interrupts enabled for btns
	P1IES |= (B0 + B1 + B2);		// edge select hi -> low trigger
	P1IFG &= ~(B0 + B1 + B2);	// clear flag bit for first use

	//Initialize Ports
	CSPort |= CS;		//Use bit to activate CE (active low) on the DAC(pin 2)

	USCIPS1 = UCBSIMO + UCBCLK;	// These two lines select UCB0CLK for P1.5
	USCIPS2 = UCBSIMO + UCBCLK;	// and select UCB0SIMO for P1.7

	UCB0CTL0 |= UCCKPL + UCMSB + UCMST + UCSYNC;

	UCB0CTL1 |= UCSSEL_2; 	// sets UCB0 will use SMCLK as the bit shift clock

	UCB0CTL1 &= ~UCSWRST;	//SPI enable

	initTimer();
	//set up timer with 100Hz initially
	setParameters(freqs[current_freq], 100);

	_enable_interrupts();

	while (1) {
		//setParameters(freqs[current_freq], 100, 50);
	}
}

/*
 * Drive_Dac
 * Drives the DAC at max possible speed
 * -note apparently at this speed (16Mhz) the buffer
 * 	is ready as soon as the command is sent, no waiting really needed
 * 	just make sure that that enable is held long enough
 */
void Drive_DAC(unsigned int level) {
	//Create the container for the total data that's going to be sent (16 bits total)
	unsigned int DAC_Word = 0;	//default is DAC off
	//check if level and duty is non-zero
	if(level && duty)
		DAC_Word = (0x1000) | (level & 0x0FFF); // 0x1000 sets DAC for Write

	CSPOut &= ~CS; //Select the DAC
	UCB0TXBUF = (DAC_Word >> 8); // Send in the upper 8 bits

	//would normally wait for buffer to flush, but don't need to at 16Mhz

	UCB0TXBUF = (unsigned char) (DAC_Word & 0x00FF); // Transmit lower byte to DAC

	//roughly the the time needed to process everything
	__delay_cycles(10);	//any less and we don't get anything

	//Clock the data in
	CSPOut |= CS;
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
 */
void setParameters(int freq, int sample) {
	switch (freq) {
	case 500:
		TA0CCR0 = cali100Samples[4];
		break;
	case 400:
		TA0CCR0 = cali100Samples[3];
		break;
	case 300:
		TA0CCR0 = cali100Samples[2];
		break;
	case 200:
		TA0CCR0 = cali100Samples[1];
		break;
	case 100:
		TA0CCR0 = cali100Samples[0];
		break;
	}
	samples = sample;
}

void toggleSqaure() {
	if (sqaure_val == 0)
		sqaure_val = 2048;
	else
		sqaure_val = 0;
}

void onSqaure() {
	sqaure_val = 2048;
}

void offSqaure() {
	sqaure_val = 0;
}

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A(void) {
	switch (state) {
	case 0:
		//This is a square wave state
		//check for duty changes first
		if (duty == 0) {
			//if duty is zero - turn off wave
			offSqaure();
		} else if (duty == 100) {
			//we're at 100 duty - just a dc;
			onSqaure();
		} else if (duty == count) {
			//we're at the duty , turn off wave
			offSqaure();
		}

		if (count == samples) {
			//reset count
			count = 0;
			//set the wave on
			onSqaure();
		}

		count++;

		//drive dac with sqaure
		Drive_DAC(sqaure_val);
		break;
	case 1:
		if (sin_pos + 1 > (samples / 2) - 1) {
			sin_pos = 0;	//reset so we can draw other half
			count ^= 1;		//toggle which part we want to draw
		} else
			sin_pos++;

		//do first half of sine wave
		if (count > 0)
			Drive_DAC(sine1_50Samples[sin_pos]);
		else
			Drive_DAC(sine2_50Samples[sin_pos]);
		break;
	case 2:
		//sawtooth
		//This is a sqaure wave state
		if (count % samples == 0) {
			saw_val = 0;
			count = 0;
		} else {
			saw_val += 20;
		}
		count++;

		//send the saw value to the DAC
		Drive_DAC(saw_val);
		break;
	}
}

void incrementFrequency() {
	if (current_freq + 1 > 4)
		current_freq = 0;
	else
		current_freq++;

	setParameters(freqs[current_freq], 100); //set it
}

void incrementState() {
	if (state + 1 > 2)
		state = 0;
	else
		state++;
	//on state change - reset count and duty
	count = 0;
	duty = 50;
}

void incrementDuty() {
	duty += 10;
	if (duty > 100)
		duty = 0;
}

//Button push interrupt ISR
#pragma vector=PORT1_VECTOR
__interrupt void ISR_PORT1(void) {
	if ((P1IFG & B0)== B0) {
		incrementFrequency();
		P1IFG &= ~B0;	//clear interrupt flag
	} else if ((P1IFG & B1)== B1) {
		incrementState();
		P1IFG &= ~B1;	//clear interrupt flag
	} else if ((P1IFG & B2)== B2) {
		//only increment duty if on sqaure state
		if(!state)
			incrementDuty();
		P1IFG &= ~B2;	//clear interrupt flag
	}
}
