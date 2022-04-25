/*
 * Lab 6.cpp
 *
 * Created: 2/28/2022 12:40:56 PM
 * Author : cfull
 */ 

#include <avr/io.h>
#include <math.h>

//Global variables
char sensor_value = 0;

int main(void)
{
	DDRB = 0xFF;//Set all port B pins as output
	DDRC = 0x00;//Set all port C pins as input
	DDRD = 1<<DDD6 | 1<<DDD5 | 1<<DDD1 | 1<<DDD0;/* Make OC0A (PD6) and OC0B (PD5) output bits (PWM bits)
	PD0 & PD1 are output bits also (connected to forward and reverse pins in H-bridge)*/
	
	//Set up ADC registers
	PRR = 0x00;//Clear power reduction ADC bit
	ADMUX = 0b01100000;//Set ref voltage to AVcc, left-justify result
	ADCSRA = 0b10000111;//Enable ADC, set prescaler to 128
	
	//PWM setup
	OCR0A = 0x00; // Load $00 into OCR0 to set initial duty cycle to 0 (motor off)
	
	TCCR0A = 1<<COM0A1 | 1<<WGM01 | 1<<WGM00; /* Set non-inverting mode on
	OC0A pin (COMA1:0 and COMB0:1 bits = bits 7:4 = 1000; Fast PWM (WGM1:0 bits = bits
	1:0 = 11) */
	
	TCCR0B = 0<<CS02 | 1<<CS01 | 1<<CS00; /* Set base PWM frequency (CS02:0 -
	bits 2-0 = 011 for prescaler of 64, for approximately 1kHz base frequency)
	PWM is now running on selected pin at selected duty cycle */
    
    while (1) 
    {
		//Read analog input
		ADCSRA = ADCSRA | 0b01000000; //Alternate code: ADCSRA |= (1<<ADSC); // Start conversion
		while ((ADCSRA & 0b00010000) == 0);
			
			sensor_value = ADCH;//Read high byte of the data
			int max_bit = 255;
			
			//If signal is within -- range, turn on -- LED
			
			//Backwards - fast
			if(sensor_value<(max_bit/5)){
				int bit_val = (int)(pow(2,0)+0.5);
				char LED = (char)bit_val;
				PORTB = ~LED;
				
				//Set M1 Forward = 0 (PD0), M1 Reverse = 1 (PD1)
				PORTD = 1<<DDD1 | 0<<DDD0;
				//~75% duty cycle
				char duty_cycle = (char)(0.75*max_bit);
				OCR0A = duty_cycle;
			}
			
			//Backwards - slow
			else if(sensor_value>=(max_bit/5) && sensor_value<((2*max_bit)/5)){
				int bit_val = (int)(pow(2,1)+0.5);
				char LED = (char)bit_val;
				PORTB = ~LED;
				
				//Set M1 Forward = 0 (PD0), M1 Reverse = 1 (PD1)
				PORTD = 1<<DDD1 | 0<<DDD0;
				//~33% duty cycle
				char duty_cycle = (char)(0.33*max_bit);
				OCR0A = duty_cycle;
			}
			
			//Stopped
			else if(sensor_value>=((2*max_bit)/5) && sensor_value<((3*max_bit)/5)){
				int bit_val = (int)(pow(2,2)+0.5);
				char LED = (char)bit_val;
				PORTB = ~LED;
				
				//Clear M1 Forward & M1 Reverse (both = 0)
				PORTD = 0<<DDD1 | 0<<DDD0;
				//0% duty cycle
				OCR0A = 0x00;
			}
			
			//Forwards - slow
			else if(sensor_value>=((3*max_bit)/5) && sensor_value<((4*max_bit)/5)){
				int bit_val = (int)(pow(2,3)+0.5);
				char LED = (char)bit_val;
				PORTB = ~LED;
				
				//Set M1 Forward = 1 (PD0), M1 Reverse = 0 (PD1)
				PORTD = 0<<DDD1 | 1<<DDD0;
				//33% duty cycle
				char duty_cycle = (char)(0.33*max_bit);
				OCR0A = duty_cycle;
			}
			
			//Forwards - fast
			else if(sensor_value>=((4*max_bit)/5) && sensor_value<=max_bit){
				int bit_val = (int)(pow(2,4)+0.5);
				char LED = (char)bit_val;
				PORTB = ~LED;
				
				//Set M1 Forward = 1 (PD0), M1 Reverse = 0 (PD1)
				PORTD = 0<<DDD1 | 1<<DDD0;
				//Duty cycle ranges from 33% at 4 V to 100% at 5 V, proportional to potentiometer
				float sens_val = (float)sensor_value;
				float duty_cycle_eqn = (0.67/51)*(sens_val-204)+0.33;
				char duty_cycle = (char)(max_bit*duty_cycle_eqn);
				OCR0A = duty_cycle;
			}
    }
}

