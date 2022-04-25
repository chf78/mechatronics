/*
 * Lab 10 - SPI.cpp
 *
 * Created: 4/1/2022 1:13:48 PM
 * Author : cfull
 */ 

#include <avr/io.h>

/* Global variables */
char data_in; //variable for holding data

/* Functions */
int send_to_MAX7221(unsigned char, unsigned char);
void wait(volatile int, volatile char);
void delay_T_msec_timer0(volatile char);
void blink(void);

int main(void)
{
	//Setup
	DDRB = 0b00101100; //Set MOSI, SCK, and SS pins as output, MISO is input
	
	//Main SPI setup
	SPCR = 0b01010001; //Enable SPI, set to Main mode, SCK = Fosc/16, MSB transmitted first
	
	//Enable decoding mode for DIG0 and DIG1
	unsigned char decode = 0b00001001;
	unsigned char digits = 0b00001111;
	send_to_MAX7221(decode,digits);
	
	//Set scan limit
	unsigned char scan_limit = 0b00001011;
	send_to_MAX7221(scan_limit,digits);
	
	//Turn on display
	unsigned char turn_on = 0b00001100;
	send_to_MAX7221(turn_on,0x01);
	
    while (1) 
    {

		blink();
		wait(2000,2);
	}
}

int send_to_MAX7221(unsigned char command, unsigned char data) {
	// Transmit the data
	PORTB &= ~(0b00000100); //(1 << PORTB2);  // Clear the SS bit to enable Secondary

	SPDR = command; //Send command to MAX 7221
	while ((SPSR & 0b10000000) == 0); //while ((SPSR & (0x1<<SPIF)) == 0) {} // Check the SPIF bit and wait for it to be set => transmit complete

	SPDR = data; //Send command to MAX 7221
	while ((SPSR & 0b10000000) == 0);//Wait for data transmission to finish

	PORTB |= 0b00000100; //PORTB |= 1 << PORTB2;  // disable Secondary (this clears SBIF flag) to end transmission
	
	return 0;
}

void wait(volatile int multiple, volatile char time_choice) {
	/* This subroutine calls others to create a delay.
		 Total delay = multiple*T, where T is in msec and is the delay created by the called function.
	
		Inputs: multiple = number of multiples to delay, where multiple is the number of times an actual delay loop is called.
		Outputs: None
	*/
	
	while (multiple > 0) {
		delay_T_msec_timer0(time_choice); 
		multiple--;
	}
} // end wait()

void delay_T_msec_timer0(volatile char choice) {
    //*** delay T ms **
    /* This subroutine creates a delay of T msec using TIMER0 with prescaler on clock, where, for a 16MHz clock:
    		for Choice = 1: T = 0.125 msec for prescaler set to 8 and count of 250 (preload counter with 5)
    		for Choice = 2: T = 1 msec for prescaler set to 64 and count of 250 (preload counter with 5)
    		for Choice = 3: T = 4 msec for prescaler set to 256 and count of 250 (preload counter with 5)
    		for Choice = 4: T = 16 msec for prescaler set to 1,024 and count of 250 (preload counter with 5)
			for Choice = Default: T = .0156 msec for no prescaler and count of 250 (preload counter with 5)
	
			Inputs: None
			Outputs: None
	*/
	
	TCCR0A = 0x00; // clears WGM00 and WGM01 (bits 0 and 1) to ensure Timer/Counter is in normal mode.
	TCNT0 = 0;  // preload value for testing on count = 250
	// preload value for alternate test on while loop: TCNT0 = 5;  // preload load TIMER0  (count must reach 255-5 = 250)
	
	switch ( choice ) { // choose prescaler
		case 1:
			TCCR0B = 0b00000010; //1<<CS01;	TCCR0B = 0x02; // Start TIMER0, Normal mode, crystal clock, prescaler = 8
		break;
		case 2:
			TCCR0B =  0b00000011; //1<<CS01 | 1<<CS00;	TCCR0B = 0x03;  // Start TIMER0, Normal mode, crystal clock, prescaler = 64
		break;
		case 3:
			TCCR0B = 0b00000100; //1<<CS02;	TCCR0B = 0x04; // Start TIMER0, Normal mode, crystal clock, prescaler = 256
		break; 
		case 4:
			TCCR0B = 0b00000101; //1<<CS02 | 1<<CS00; TCCR0B = 0x05; // Start TIMER0, Normal mode, crystal clock, prescaler = 1024
		break;
		default:
			TCCR0B = 0b00000001; //1<<CS00; TCCR0B = 0x01; Start TIMER0, Normal mode, crystal clock, no prescaler
		break;
	}
	
	while (TCNT0 < 0xFA); // exits when count = 250 (requires preload of 0 to make count = 250)
	// alternate test on while loop: while ((TIFR0 & (0x1<<TOV0)) == 0); // wait for TOV0 to roll over:
	// How does this while loop work?? See notes
	
	TCCR0B = 0x00; // Stop TIMER0
	//TIFR0 = 0x1<<TOV0;  // Alternate while loop: Clear TOV0 (note that this is a nonintuitive bit in that it is cleared by writing a 1 to it)
	
} // end delay_T_msec_timer0()

void blink(void){
	unsigned char set_digit_3 = 0b00000100;
	unsigned char set_digit_2 = 0b00000011;
	unsigned char set_digit_1 = 0b00000010;
	unsigned char set_digit_0 = 0b00000001;
	
	send_to_MAX7221(set_digit_3,1);
	send_to_MAX7221(set_digit_2,2);
	send_to_MAX7221(set_digit_1,3);
	send_to_MAX7221(set_digit_0,4);
	
	wait(250,2);
	
	send_to_MAX7221(set_digit_3,5);
	send_to_MAX7221(set_digit_2,6);
	send_to_MAX7221(set_digit_1,7);
	send_to_MAX7221(set_digit_0,8);
	
	wait(250,2);
}