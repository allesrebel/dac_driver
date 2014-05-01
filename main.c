#include <msp430.h> 
//#include "lcd_driver4bit.h"

void Drive_DAC(unsigned int level);
void setParameters(int freq, int samples, float duty);
void initTimer();

//Debugging functions
void incrementFrequency();
void incrementState();

/*
 * Pin configs - to make it easier to change if needed
 */
#define CS		BIT4
#define CSPOut	P1OUT
#define CSPort	P1DIR

//USCI General Port Info
#define USCIPS1	P1SEL	//USCI Pin mode select
#define USCIPS2	P1SEL2	//USCI Pin mode select 2
//UCA0 PORT Defs
#define UCASOMI	BIT1	//isn't really used to communicate with DAC
#define UCASIMO	BIT2
#define UCACLK	BIT4

//UCB0 PORT Defs
#define UCBSOMI	BIT6	//isn't really used to communicate with DAC
#define UCBSIMO	BIT7
#define UCBCLK	BIT5

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
volatile unsigned int SIN_Values[50] = { 1024, 1152, 1278, 1400, 1517, 1625,
		1724, 1813, 1888, 1950, 1997, 2029, 2045, 2045, 2029, 1997, 1950, 1888,
		1813, 1724, 1625, 1517, 1400, 1278, 1152, 1024, 895, 769, 647, 530, 422,
		323, 234, 159, 97, 50, 18, 2, 2, 18, 50, 97, 159, 234, 323, 422, 530,
		647, 769, 895 }; // Sin values rounded to nearest int.

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

	USCIPS1 = UCBSIMO + UCBCLK;	// These two lines select UCB0CLK for P1.5
	USCIPS2 = UCBSIMO + UCBCLK;	// and select UCB0SIMO for P1.7

	UCB0CTL0 |= UCCKPL + UCMSB + UCMST + UCSYNC;

	UCB0CTL1 |= UCSSEL_2; 	// sets UCB0 will use SMCLK as the bit shift clock

	UCB0CTL1 &= ~UCSWRST;	//SPI enable

	initTimer();
	//set up timer with 100Hz initially
	setParameters(freqs[current_freq], 100, 50);

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
	unsigned int DAC_Word = (0x1000) | (level & 0x0FFF); // 0x1000 sets DAC for Write

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

		//drive dac with sqaure
		Drive_DAC(sqaure_val);
		break;
	case 1:
		if (sin_pos + 1 > 49)
			sin_pos = 0;
		else
			sin_pos++;

		//drive Dac with sine val
		Drive_DAC(SIN_Values[sin_pos]);
		break;
	case 2:
		//sawtooth

		break;
	}
}

void incrementFrequency() {
	if (current_freq + 1 > 4)
		current_freq = 0;
	else
		current_freq++;

	setParameters(freqs[current_freq], 100, 50); //set it
}

void incrementState() {
	if (state + 1 > 2)
		state = 0;
	else
		state++;
}

//Button push interrupt ISR
#pragma vector=PORT1_VECTOR
__interrupt void ISR_PORT1(void) {
	if ((P1IFG & BIT3)== BIT3) {
		incrementState();

		P1IFG &= ~BIT3;	//clear interrupt flag
	}
}
