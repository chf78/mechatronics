/*
 * Lab 8.cpp
 *
 * Created: 3/27/2022 4:12:19 PM
 * Author : cfull
 */ 

#include <avr/io.h>

/*Functions*/
void AD_setup(void);
void initialize_usart(void); // function to set up USART
void transmit_data_usart(int); // function to transmit a byte by USART

/*Global variables*/
int data = 0;

int main(void)
{
	
	//Setup
	DDRC = 0x00; //Set potentiometer pin (PC1) to input
	DDRD = 0xFF; // Set transmit pin (PD1) to output
	AD_setup();
	initialize_usart(); // Initialize the USART with desired parameters
	
    while (1) 
    {
		//Read analog input
		ADCSRA = ADCSRA | 0b01000000;// Start conversion
		while ((ADCSRA & 0b00010000) == 0);
			
		data = ADCH;//Read high byte of the data

		transmit_data_usart(data);
    }
	return 0;
}

void AD_setup(void){
	 //Set up ADC registers
	 PRR = 0x00;//Clear power reduction ADC bit
	 ADMUX = 0b01100001;//Set ref voltage to AVcc, left-justify result
	 ADCSRA = 0b10000111;//Enable ADC, set prescaler to 128
}

void initialize_usart(void) // function to set up USART
{
	UCSR0B = (1<<TXEN0); // enable serial transmission
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00) ; // Asynchronous mode, 8-bit data; no parity; 1 stop bit
	UBRR0L = 0x67; // 9,600 baud if Fosc = 16MHz
}

void transmit_data_usart(int data) // Function to transmit data
{
	while (!(UCSR0A & (1<<UDRE0))); // Poll to make sure transmit buffer is ready,// then send data
	UDR0 = data;
}
