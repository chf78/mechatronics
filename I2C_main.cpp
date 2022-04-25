/*
 * Lab 11 - I2C Communication.cpp
 *
 * Created: 4/4/2022 7:17:14 PM
 * Author : cfull
 */ 

#include <avr/io.h>

/** Function Declarations */
void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_repeated_start(void);
void i2c_write_to_address(unsigned char address);
void i2c_read_from_address(unsigned char address);
void i2c_write_data(unsigned char data);
unsigned char i2c_read_data(unsigned char ack);
unsigned char convert_to_fahrenheit(unsigned char upper_byte, unsigned char lower_byte);
void delay_T_msec_timer0(volatile char choice);
void wait(volatile int multiple, volatile char time_choice);
int send_to_MAX7221(unsigned char command, unsigned char data);

int main(void)
{
	unsigned char temp, upper_byte, lower_byte;
	
    //ATmega setup
	DDRB = 0b00101100; //Set MOSI, SCK, and SS pins as output, MISO is input
	
	//SPI setup for LED displays
	SPCR = 0b01010001; //Enable SPI, set to Main mode, SCK = Fosc/16, MSB transmitted first
	
	//Enable decoding mode for DIG0 and DIG1
	unsigned char decode = 0b00001001;
	unsigned char digits = 0b00000011;
	send_to_MAX7221(decode,digits);
	
	//Set scan limit
	unsigned char scan_limit = 0b00001011;
	send_to_MAX7221(scan_limit,digits);
	
	//Turn on display
	unsigned char turn_on = 0b00001100;
	send_to_MAX7221(turn_on,0x01);
	
	// I2C (TWI) Setup
	//If needed, turn on TWI power: PRR = PRR & 0b01111111; // Ensure that TWI is powered on (PRTWI = 0)
	DDRC = 0b00000000; // Define all PORTC bits as input (specifically PC4 and PC5)
	PORTC = PORTB | 0b00110000;  // set internal pull-up resistors on SCL and SDA lines (PC5 and PC4) of I2C bus
	i2c_init();
	
	//Set resolution for temperature sensor
	i2c_start();
	i2c_write_to_address(0x18);
	i2c_write_data(0x08); //Write to resolution register
	i2c_write_data(0b00000011); //Set resolution as -- degrees C
	i2c_stop();
	
	
    while (1) 
    {
		//Read data from temperature sensor
		i2c_start();
		i2c_write_to_address(0x18);
		i2c_write_data(0x05); //Ambient temperature register is 0x05
		i2c_repeated_start();
		i2c_read_from_address(0x18);
		upper_byte = i2c_read_data(1); //Read upper byte with ACK
		lower_byte = i2c_read_data(0); //Read lower byte with NO_ACK
		i2c_stop();
		
		//Convert data to degrees F
		temp = convert_to_fahrenheit(upper_byte,lower_byte);
		int temp_tens = int(temp)/10;
		int temp_ones = int(temp)%10;
		unsigned char temp_first_digit = char(temp_tens);
		unsigned char temp_second_digit = char(temp_ones); 
		
		//Set month first digit
		unsigned char set_dig_1 = 0b00000010;
		send_to_MAX7221(set_dig_1,temp_first_digit);
		
		//Set month second digit
		unsigned char set_dig_0 = 0b00000001;
		send_to_MAX7221(set_dig_0,temp_second_digit);
		
		wait(500,2);
    }
}

void i2c_init(void) { // initialize i2c
	TWSR = 0b00000000; // prescaler is set to 1
	TWBR = 72; // Put 72 into TWBR to define SCL frequency as 100kHz for 16MHz oscillator
	TWCR = 0b00000100; // TWEN = 1 (enable TWI)
}

void i2c_start(void) { // send start command
	//while (!(TWCR & (1<<TWINT))); //while (!(TWCR & 0b10000000));   // wait for idle condition -- TWINT bit must be high to proceed -- not needed if single main is used
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);  //TWCR | 0b10100100;       // initiate START condition -- write 1 to TWINT to clear it and initiate action, set TWSTA bit, set TWEN bit
	while (!(TWCR & (1<<TWINT))); //while (!(TWCR & 0b10000000));   // wait for action to finish (poll TWINT bit)
	// if ((TWSR & 0xF8) != START) // error checking -- need to predefine START = 0x08 and ERROR() function.
	// ERROR();
}

void i2c_stop(void) { // send stop command
	while (!(TWCR & (1<<TWINT))); //while (!(TWCR & 0b10000000)) ;  // wait for action to finish (poll TWINT bit)
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO); //TWCR = TWCR | 0b10010100;       // initiate STOP condition -- write 1 to TWINT to clear it and initiate action, set TWSTO bit and set TWEN bit
}

void i2c_repeated_start(void) {
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);  //TWCR | 0b10100100;       // initiate START condition -- write 1 to TWINT to clear it and initiate action, set TWSTA bit, set TWEN bit
	while (!(TWCR & (1<<TWINT))); //while (!(TWCR & 0b10000000));   // wait for action to finish (poll TWINT bit)
	// if ((TWSR & 0xF8) != START) // error checking -- need to predefine START = 0x10 and ERROR() function.
	// ERROR();
}

void i2c_write_to_address( unsigned char address) { //Write an address byte to the I2C2 bus in form of SLA_W (address to write to)
	unsigned char SLA_W = address<<1; // create SLA_W byte by shifting address and leaving R/W bit clear
	while (!(TWCR & (1<<TWINT))); //while (!(TWCR & 0b10000000)) ;  // wait for idle condition -- TWINT bit must be high to proceed
	TWDR = SLA_W;       // Load TWDR with address plus R/W bit
	TWCR = (1<<TWINT) | (1<<TWEN); //TWCR = TWCR | 0b10000100;       // initiate Write -- write 1 to TWINT to clear it and initiate action, and set TWEN
	while (!(TWCR & (1<<TWINT))); //while (!(TWCR & 0b10000000)) ;  // wait for action to finish (poll TWINT bit)
	// if ((TWSR & 0xF8) != MT_SLA_ACK) // error checking -- need to predefine MT_SLA_ACK and ERROR() function depending on possible outcomes: 0x18, 0x20, or 0x38.
	//ERROR();
}

void i2c_read_from_address(unsigned char address) { //Write an address byte to the I2C bus in form of SLA_R (address to read from)
	unsigned char SLA_R = address<<1 | 1; // create SLA_R byte by shifting address and setting R/W bit
	while (!(TWCR & (1<<TWINT))); //while (!(TWCR & 0b10000000)) ;  // wait for idle condition -- TWINT bit must be high to proceed
	TWDR = SLA_R;       // Load TWDR with address plus R/W bit
	TWCR = (1<<TWINT) | (1<<TWEN); //TWCR = TWCR | 0b10000100;       // initiate Write -- write 1 to TWINT to clear it and initiate action, and set TWEN
	while (!(TWCR & (1<<TWINT))); //while (!(TWCR & 0b10000000)) ;  // wait for action to finish (poll TWINT bit)
	// if ((TWSR & 0xF8) != MR_SLA_ACK) // error checking -- need to predefine MR_SLA_ACK and ERROR() function depending on possible outcomes: 0x38, 0x40, or 0x48.
	//ERROR();
}

void i2c_write_data( unsigned char data) { //Write data byte to the I2C2 bus
	while (!(TWCR & (1<<TWINT))); //while (!(TWCR & 0b10000000)) ;  // wait for idle condition -- TWINT bit must be high to proceed
	TWDR = data;       // Load TWDR with data to be sent
	TWCR = (1<<TWINT) | (1<<TWEN); //TWCR = TWCR | 0b10000100;       // initiate Write -- write 1 to TWINT to clear it and initiate action, and set TWEN
	while (!(TWCR & (1<<TWINT))); //while (!(TWCR & 0b10000000)) ;  // wait for action to finish (poll TWINT bit)
	// if ((TWSR & 0xF8) != MT_DATA_ACK) // error checking -- need to predefine MT_DATA_ACK and ERROR() function depending on possible outcomes: 0x28 or 0x30.
	//ERROR();
}


unsigned char i2c_read_data(unsigned char ACK) { //Read a byte of data from a secondary on the I2C2 bus
	unsigned char data;
	while (!(TWCR & (1<<TWINT))); //while (!(TWCR & 0b10000000)) ;  // wait for idle condition -- TWINT bit must be high to proceed
	if (ACK) // check for whether ACK or NO_ACK should be sent upon receipt of byte from secondary
	TWCR = (1<<TWINT) | (1<<TWEA) | (1<<TWEN); //TWCR = TWCR | 0b11000100;       // initiate Read with ACK -- write 1 to TWINT to clear it and initiate action, and set TWEA and TWEN
	else
	TWCR = (1<<TWINT) | (1<<TWEN); //TWCR = TWCR | 0b10000100;       // initiate Read with NO_ACK-- write 1 to TWINT to clear it and initiate action, and set TWEN
	while (!(TWCR & (1<<TWINT))); //while (!(TWCR & 0b10000000)) ;  // wait for action to finish (poll TWINT bit)
	// if ((TWSR & 0xF8) != MR_SLA_ACK) // error checking -- need to predefine MR_SLA_ACK and ERROR() function depending on possible outcomes: 0x50 or 0x58.
	//ERROR();
	//If multiple bytes are to be read, this function can be repeated with proper ACK or NO_ACK until done.
	data = TWDR;  // read the received data from secondary
	return(data);
}

int send_to_MAX7221(unsigned char command, unsigned char data){
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

unsigned char convert_to_fahrenheit(unsigned char upper_byte, unsigned char lower_byte){
	unsigned char temp;
	//Convert data to Celsius
	upper_byte = upper_byte & 0x1F; //Clear upper 3 flag bits
	if((upper_byte & 0x10)==0x10){ //If T<0 degrees C
		upper_byte = upper_byte & 0x0F;
		temp = 256 - (upper_byte*16 + lower_byte/16);
	}
	else{ //If T>0 degrees C
		temp = (upper_byte*16 + lower_byte/16);
	}
	temp = temp*9*0.2 + 32; //Convert from celsius to fahrenheit ( temp*(9/5)+32 )
	return temp;
}