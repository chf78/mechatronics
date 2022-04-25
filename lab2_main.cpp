/*
 * GccApplication1.cpp
 *
 * Created: 2/2/2022 12:07:49 PM
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
	DDRD=0b00000000;//Set all PORTD pins to input (only using PD2)
	PIND=0b00000100;//Set PD2 as high when switch is open
    
    while (1) 
    {
		unsigned char i=0;
		while (i<7){
			if(!(PIND & 0b00000100)){
				PORTC = 0b11111111;
				}
				
			else{
				unsigned char n=i;
				//If i<4, go in normal direction
				if(i<4){
					PORTC ^= 1<<n;
					wait(500);
					PORTC ^= 1<<n;}
					
				//Else, if i>=4, go backwards
				if(i>=4 && i<6){
					PORTC ^= 1<<(6-n);
					wait(500);
					PORTC ^= 1<<(6-n);}
				i++;	
				if(i==6){i=0;}
				}
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