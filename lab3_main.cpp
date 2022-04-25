/*
 * Lab 3.cpp
 *
 * Created: 2/6/2022 10:06:06 PM
 * Author : cfull
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

//** Functions **//
void wait(volatile int multiple);

int main(void)
{
		//Setup
		DDRC=0xFF;//Set all PORTC pins to output
		PORTC=0b00001111;//Turn off all pins on PORTC (assuming wired active low)
		DDRD=0b00000000;//Set all PORTD pins to input (only using PD2 & PD3)
		PIND=0b00001100;//Set PD2 & PD3 as high when switch is open
		
		//Interrupts
		EICRA = 1<<ISC01 | 0<<ISC00 | 1<<ISC11 | 0<<ISC10;
		EIMSK = 1<<INT1 | 1<<INT0;
		sei();//Gloabl Interrupt
		
		while (1)
		{
			unsigned char i=0;
			while (i<7){
					unsigned char n=i;
					//If i<4, go in normal direction
					if(i<4){
						PORTC ^= 1<<n;
						wait(500);
						PORTC ^= 1<<n;}
					
					//Else, if i>=4, go backwards
					else{
						PORTC ^= 1<<(6-n);
						wait(500);
						PORTC ^= 1<<(6-n);}
					i++;
					if(i==6){i=0;}
					}
		}
}

//Subroutine for wait time
void wait(volatile int N) {
	// This subroutine creates a delay equal to N msec
	while (N > 0) {
		TCCR0A = 0x00; // clears WGM00 and WGM01 (bits 0 and 1) to ensure Timer/Counter is in normal mode.
		TCNT0 = 0;  // preload value for testing on count = 250
		TCCR0B = 0b00000011; //1<<CS01 | 1<<CS00;	TCCR0B = 0x03;  // Start TIMER0, Normal mode, 16 MHz crystal clock, prescaler = 64
		while (TCNT0 < 0xFA); // exits when count = 250 (which is the count required to create a 1 msec delay with 16MHz clock and prescaler of 64)
		TCCR0B = 0x00; // Stop TIMER0
		N--;
	}
} // end wait()

//Subroutine for INT0 interrupt
ISR(INT0_vect){
		//Save important info
		unsigned char LEDS;
		LEDS = PORTC;
		unsigned char count;
		count = TCNT0;
		
		PORTC = 0xFF;//Turn off all LEDs for 2 sec
		wait(1000);
		
		unsigned int spot;
		for(spot=255;spot>=(255-16);spot--){
			PORTC = spot;
			wait(250);	
		}
		
		PORTC = LEDS;
		TCNT0 = count;
		EIFR = 0b00000001;//Clear INT0 flag, leave INT1 flag in case it was triggered
		TCCR0B = 0b00000011;//Restart timer
}

ISR(INT1_vect){
	unsigned char LEDS;
	unsigned char count;
	LEDS = PORTC;
	count = TCNT0;
	
	PORTC = 0x00;//Turn on all LEDs for 2 sec
	wait(1000);
	
	int n;
	for(n=4;n>0;n--){
		PORTC ^= 1<<n;
		wait(500);
	}
	
	PORTC = LEDS;
	TCNT0 = count;
	EIFR = 0b00000001;//Clear INT0 flag, leave INT1 flag in case it was retriggered
	TCCR0B = 0b00000011;//Restart timer
}	