/* dog feeder mega circuit */

#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>

/* Functions */
void AD_conversion_setup(void);
void SPI_setup(void);
int send_to_MAX7221(unsigned char, unsigned char);
void wait(volatile int, volatile char);
void delay_T_msec_timer0(volatile char);
int set_hours(unsigned char);
float set_food(unsigned char);
void setting_change(void);
int set_hours_display (void);
float set_meal_display (void);
float cups_to_byte(float);
float fill_bowl_halfcup(float,float);
float fill_bowl_1cup(float,float);
float fill_bowl_1andhalfcup(float,float);
float fill_bowl_2cup(float,float);
float fill_bowl_2andhalfcup(float,float);
float fill_bowl_3cup(float,float);
float fill_bowl_3andhalfcup(float,float);
float fill_bowl_4cup(float,float);
void step_CW( void );
void step_CCW( void );
void door(int);
void door_open(void);
void door_close(void);

/* Global variables */
char set_time_hours = 0;
char set_food_amount = 0;
unsigned char set_digit_3 = 0b00000100;
unsigned char set_digit_2 = 0b00000011;
unsigned char set_digit_1 = 0b00000010;
unsigned char set_digit_0 = 0b00000001;
unsigned char turn_on = 0b00001100;
unsigned char disp_test = 0b00001111;
int max_bit = 255;
int wait_time = 0;
float meal_size = 0;
float cups = 0;
char weight_sensor = 0;
int phase_step = 1;
char proximity = 0;

bool found_wait = 0;
bool found_meal = 0;
bool found_exit = 0;

int main(void){
	
	//------------------------------ I/O port setup -----------------------------------
	DDRB = 0b00101100; //Set MOSI, SCK, and SS pins as output, MISO is input
	DDRC = 0x00; // PC0 is potentiometer, PC1-PC2 are switches, PC5 is weight sensor
	DDRD = 0b11110000; // set PD4-PD7 to output for stepper motor
	//-------------------------- end I/O port setup --------------------------------------
	
	// Set up Interrupts
	EICRA = 1<<ISC01 | 1<<ISC00;	// Trigger INT0 on rising edge
	EIMSK = 1<<INT0;	// Enable INT0
	sei();	//Enable Global Interrupt
	
	//set up A/D conversion for potentiometer
	AD_conversion_setup();
	
	//set up SPI for MAX chip
	SPI_setup();
	
	//turn off all displays
	send_to_MAX7221(turn_on,0x00);
	
	
	
	while(1){
		
		
		
		
		while(!found_exit){
		//---------------------------------- GET WAIT TIME AND MEAL SIZE FROM DIAL --------------------------------------------------
			
			//-------------------------- press and hold button 1 to set wait time -------------
			if(!(PINC & 0b00000010)){
				wait_time = set_hours_display();
				//wait(10000,2);
				found_wait = 1;
			}
			//-------------------------- release button 1 ----------------------------------------
			
			//-------------------------- press and hold button 2 to set meal size --------------
			else if(!(PINC & 0b00000100)){
				meal_size = set_meal_display();
				cups = meal_size;//save meal size value in cups for later use
				//wait(10000,2);
				found_meal = 1;
			}
			//-------------------------- release button 2 ----------------------------------------
			else if(found_wait && found_meal){
				found_exit = 1;
			}
		}
		
		wait(5000,2);
		
		//-------------------------------- CONVERT MEAL SIZE TO A BYTE VALUE -------------------------------------
		//float cups = meal_size;//save meal size value in cups for later use
		meal_size = cups_to_byte(meal_size);//calculate equivalent of meal size in byte value
		
		/*		
		//-------------------------------- A/D CONVERSION FOR WEIGHT SENSOR ---------------------------------------------------
		
		ADCSRA = ADCSRA | 0b01000000;
		while ((ADCSRA & 0b00010000) == 0);
		weight_sensor = ADCH;
		
		weight_sensor = float(weight_sensor);
		
		*/
		
		
		weight_sensor = 0;
		
		//---------------------------- DETERMINE WHICH FUNCTION TO CALL BASED ON CUPS -----------------------------------------
		float fill_amount = 0;
		int i = 0;
		float meal_options_cups[] = {0,0.5,1,1.5,2,2.5,3,3.5,4};

		if(cups == meal_options_cups[i]){
			//call zero cups function
			fill_amount = 0;
		}
		else if(cups == meal_options_cups[i+1]){
			//call 0.5 cups function
			fill_amount = fill_bowl_halfcup(weight_sensor,meal_size);
		}
		else if(cups == meal_options_cups[i+2]){
			//call 1 cups function
			fill_amount = fill_bowl_1cup(weight_sensor,meal_size);
		}
		else if(cups == meal_options_cups[i+3]){
			//call 1.5 cups function
			fill_amount = fill_bowl_1andhalfcup(weight_sensor,meal_size);
		}
		else if(cups == meal_options_cups[i+4]){
			//call 2 cups function
			fill_amount = fill_bowl_2cup(weight_sensor,meal_size);
		}
		else if(cups == meal_options_cups[i+5]){
			//call 2.5 cups function
			fill_amount = fill_bowl_2andhalfcup(weight_sensor,meal_size);
		}
		else if(cups == meal_options_cups[i+6]){
			//call 3 cups function
			fill_amount = fill_bowl_3cup(weight_sensor,meal_size);
		}
		else if(cups == meal_options_cups[i+7]){
			//call 3.5 cups function
			fill_amount = fill_bowl_3andhalfcup(weight_sensor,meal_size);
		}
		else if(cups == meal_options_cups[i+8]){
			//call 4 cups function
			fill_amount = fill_bowl_4cup(weight_sensor,meal_size);
		}
		
		//-------------------------- CALCULATE DISPENSING TIME BASED ON FILL AMOUNT --------------------------------------------------
		int disp_time = 0;
		int j = 0;
		bool found = 0;
		//array of dispensing times associated w/ each food amount
		float meal_options_time[][2] = {{0,0},{0.5,2000},{1,3000},{1.5,4000},{2,15000},{2.5,20000},{3,25000},{3.5,27000},{4,30000}};

		while(!found){
			if(fill_amount == meal_options_time[j][0]){
				disp_time = meal_options_time[j][1];
				found = 1;
			}
			j++;
		}
		
		//--------------------------------- CALL FUNCTIONS FOR DOOR AND WAIT TIME BETWEEN MEALS ----------------------------------
		door(disp_time); //open door for time determined by food amount to be dispensed
		
		wait(wait_time,2); //wait until next meal
	}
	
	//}
	
	return 0;
	
	
}

void AD_conversion_setup(void){
	//Set up AD conversion registers
	PRR = 0x00;//Clear power reduction ADC bit
	ADMUX = 0b01100000;//Set ref voltage to AVcc, left-justify result
	ADCSRA = 0b10000111;//Enable ADC, set prescaler to 128
}

void SPI_setup(void){
	//Main SPI setup
	SPCR = 0b01010001; //Enable SPI, set to Main mode, SCK = Fosc/16, MSB transmitted first
	
	//Enable decoding mode for DIG0-DIG3
	unsigned char decode = 0b00001001;
	unsigned char digits = 0b00001111;
	send_to_MAX7221(decode,digits);
	
	//Set scan limit
	unsigned char scan_limit = 0b00001011;
	send_to_MAX7221(scan_limit,digits);
	
	//Turn on displays
	unsigned char turn_on = 0b00001100;
	send_to_MAX7221(turn_on,0x01);
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

int set_hours(unsigned char set_time_hours){

	if(set_time_hours<=max_bit/7){
		send_to_MAX7221(set_digit_3,0);
		send_to_MAX7221(set_digit_2,5);
		
		if(set_time_hours<=(max_bit/28)){
			send_to_MAX7221(set_digit_1,0);
			send_to_MAX7221(set_digit_0,0);
			//wait_time = 5 hours --> 5 sec delay
			wait_time = 5000;
		}
		else if(set_time_hours>(max_bit/28) && set_time_hours<=(2*max_bit/28)){
			send_to_MAX7221(set_digit_1,1);
			send_to_MAX7221(set_digit_0,5);
			//wait_time = 5 hours 15 min --> 5.25 sec delay
			wait_time = 5250;
		}
		else if(set_time_hours>(2*max_bit/28) && set_time_hours<=(3*max_bit/28)){
			send_to_MAX7221(set_digit_1,3);
			send_to_MAX7221(set_digit_0,0);
			//wait_time = 5 hours 30 min --> 5.5 sec delay
			wait_time = 5500;
		}
		else if(set_time_hours>(3*max_bit/28) && set_time_hours<=(4*max_bit/28)){
			send_to_MAX7221(set_digit_1,4);
			send_to_MAX7221(set_digit_0,5);
			//wait_time = 5 hours 45 min --> 5.75 sec delay
			wait_time = 5750;
		}
	}
	else if(set_time_hours>max_bit/7 && set_time_hours<=(2*max_bit/7)){
		send_to_MAX7221(set_digit_3,0);
		send_to_MAX7221(set_digit_2,6);
		
		if(set_time_hours>(4*max_bit/28) && set_time_hours<=(5*max_bit/28)){
			send_to_MAX7221(set_digit_1,0);
			send_to_MAX7221(set_digit_0,0);
			//wait_time = 6 hours --> 6 sec delay
			wait_time = 6000;
		}
		else if(set_time_hours>(5*max_bit/28) && set_time_hours<=(6*max_bit/28)){
			send_to_MAX7221(set_digit_1,1);
			send_to_MAX7221(set_digit_0,5);
			//wait_time = 6 hours 15 min --> 6.25 sec delay
			wait_time = 6250;
		}
		else if(set_time_hours>(6*max_bit/28) && set_time_hours<=(7*max_bit/28)){
			send_to_MAX7221(set_digit_1,3);
			send_to_MAX7221(set_digit_0,0);
			//wait_time = 6 hours 30 min --> 6.5 sec delay
			wait_time = 6500;
		}
		else if(set_time_hours>(7*max_bit/28) && set_time_hours<=(8*max_bit/28)){
			send_to_MAX7221(set_digit_1,4);
			send_to_MAX7221(set_digit_0,5);
			//wait_time = 6 hours 45 min --> 6.75 sec delay
			wait_time = 6750;
		}
	}
	else if(set_time_hours>(2*max_bit/7) && set_time_hours<=(3*max_bit/7)){
		send_to_MAX7221(set_digit_3,0);
		send_to_MAX7221(set_digit_2,7);
		
		if(set_time_hours>(8*max_bit/28) && set_time_hours<=(9*max_bit/28)){
			send_to_MAX7221(set_digit_1,0);
			send_to_MAX7221(set_digit_0,0);
			//wait_time = 7 hours --> 7 sec delay
			wait_time = 7000;
		}
		else if(set_time_hours>(9*max_bit/28) && set_time_hours<=(10*max_bit/28)){
			send_to_MAX7221(set_digit_1,1);
			send_to_MAX7221(set_digit_0,5);
			//wait_time = 7 hours 15 min --> 7.25 sec delay
			wait_time = 7250;
		}
		else if(set_time_hours>(10*max_bit/28) && set_time_hours<=(11*max_bit/28)){
			send_to_MAX7221(set_digit_1,3);
			send_to_MAX7221(set_digit_0,0);
			//wait_time = 7 hours 30 min --> 7.5 sec delay
			wait_time = 7500;
		}
		else if(set_time_hours>(11*max_bit/28) && set_time_hours<=(12*max_bit/28)){
			send_to_MAX7221(set_digit_1,4);
			send_to_MAX7221(set_digit_0,5);
			//wait_time = 7 hours 45 min --> 7.75 sec delay
			wait_time = 7750;
		}
	}
	else if(set_time_hours>(3*max_bit/7) && set_time_hours<=(4*max_bit/7)){
		send_to_MAX7221(set_digit_3,0);
		send_to_MAX7221(set_digit_2,8);
		
		if(set_time_hours>(12*max_bit/28) && set_time_hours<=(13*max_bit/28)){
			send_to_MAX7221(set_digit_1,0);
			send_to_MAX7221(set_digit_0,0);
			//wait_time = 8 hours --> 8 sec delay
			wait_time = 8000;
		}
		else if(set_time_hours>(13*max_bit/28) && set_time_hours<=(14*max_bit/28)){
			send_to_MAX7221(set_digit_1,1);
			send_to_MAX7221(set_digit_0,5);
			//wait_time = 8 hours 15 min --> 8.25 sec delay
			wait_time = 8250;
		}
		else if(set_time_hours>(14*max_bit/28) && set_time_hours<=(15*max_bit/28)){
			send_to_MAX7221(set_digit_1,3);
			send_to_MAX7221(set_digit_0,0);
			//wait_time = 8 hours 30 min --> 8.5 sec delay
			wait_time = 8500;
		}
		else if(set_time_hours>(15*max_bit/28) && set_time_hours<=(16*max_bit/28)){
			send_to_MAX7221(set_digit_1,4);
			send_to_MAX7221(set_digit_0,5);
			//wait_time = 8 hours 45 min --> 8.75 sec delay
			wait_time = 8750;
		}
	}
	else if(set_time_hours>(4*max_bit/7) && set_time_hours<=(5*max_bit/7)){
		send_to_MAX7221(set_digit_3,0);
		send_to_MAX7221(set_digit_2,9);
		
		if(set_time_hours>(16*max_bit/28) && set_time_hours<=(17*max_bit/28)){
			send_to_MAX7221(set_digit_1,0);
			send_to_MAX7221(set_digit_0,0);
			//wait_time = 9 hours --> 9 sec delay
			wait_time = 9000;
		}
		else if(set_time_hours>(17*max_bit/28) && set_time_hours<=(18*max_bit/28)){
			send_to_MAX7221(set_digit_1,1);
			send_to_MAX7221(set_digit_0,5);
			//wait_time = 9 hours 15 min --> 9.25 sec delay
			wait_time = 9250;
		}
		else if(set_time_hours>(18*max_bit/28) && set_time_hours<=(19*max_bit/28)){
			send_to_MAX7221(set_digit_1,3);
			send_to_MAX7221(set_digit_0,0);
			//wait_time = 9 hours 30 min --> 9.5 sec delay
			wait_time = 9500;
		}
		else if(set_time_hours>(19*max_bit/28) && set_time_hours<=(20*max_bit/28)){
			send_to_MAX7221(set_digit_1,4);
			send_to_MAX7221(set_digit_0,5);
			//wait_time = 9 hours 45 min --> 9.75 sec delay
			wait_time = 9750;
		}
	}
	else if(set_time_hours>(5*max_bit/7) && set_time_hours<=(6*max_bit/7)){
		send_to_MAX7221(set_digit_3,1);
		send_to_MAX7221(set_digit_2,0);
		
		if(set_time_hours>(20*max_bit/28) && set_time_hours<=(21*max_bit/28)){
			send_to_MAX7221(set_digit_1,0);
			send_to_MAX7221(set_digit_0,0);
			//wait_time = 10 hours --> 10 sec delay
			wait_time = 10000;
		}
		else if(set_time_hours>(21*max_bit/28) && set_time_hours<=(22*max_bit/28)){
			send_to_MAX7221(set_digit_1,1);
			send_to_MAX7221(set_digit_0,5);
			//wait_time = 10 hours 15 min --> 10.25 sec delay
			wait_time = 10250;
		}
		else if(set_time_hours>(22*max_bit/28) && set_time_hours<=(23*max_bit/28)){
			send_to_MAX7221(set_digit_1,3);
			send_to_MAX7221(set_digit_0,0);
			//wait_time = 10 hours 30 min --> 10.5 sec delay
			wait_time = 10500;
		}
		else if(set_time_hours>(23*max_bit/28) && set_time_hours<=(24*max_bit/28)){
			send_to_MAX7221(set_digit_1,4);
			send_to_MAX7221(set_digit_0,5);
			//wait_time = 10 hours 45 min --> 10.75 sec delay
			wait_time = 10750;
		}
	}
	else if(set_time_hours>(6*max_bit/7) && set_time_hours<max_bit){
		send_to_MAX7221(set_digit_3,1);
		send_to_MAX7221(set_digit_2,1);
		
		if(set_time_hours>(24*max_bit/28) && set_time_hours<=(25*max_bit/28)){
			send_to_MAX7221(set_digit_1,0);
			send_to_MAX7221(set_digit_0,0);
			//wait_time = 11 hours --> 11 sec delay
			wait_time = 11000;
		}
		else if(set_time_hours>(25*max_bit/28) && set_time_hours<=(26*max_bit/28)){
			send_to_MAX7221(set_digit_1,1);
			send_to_MAX7221(set_digit_0,5);
			//wait_time = 11 hours 15 min --> 11.25 sec delay
			wait_time = 11250;
		}
		else if(set_time_hours>(26*max_bit/28) && set_time_hours<=(27*max_bit/28)){
			send_to_MAX7221(set_digit_1,3);
			send_to_MAX7221(set_digit_0,0);
			//wait_time = 11 hours 30 min --> 11.5 sec delay
			wait_time = 11500;
		}
		else if(set_time_hours>(27*max_bit/28) && set_time_hours<max_bit){
			send_to_MAX7221(set_digit_1,4);
			send_to_MAX7221(set_digit_0,5);
			//wait_time = 11 hours 45 min --> 11.75 sec delay
			wait_time = 11750;
		}
	}
	else if(set_time_hours == max_bit){
		send_to_MAX7221(set_digit_3,1);
		send_to_MAX7221(set_digit_2,2);
		send_to_MAX7221(set_digit_1,0);
		send_to_MAX7221(set_digit_0,0);
		//wait_time = 12 hours --> 12 sec delay
		wait_time = 12000;
	}
	return wait_time;
}

float set_food(unsigned char set_food_amount){
	
	//Currently assuming max amount of food per meal is 4 cups 
	if(set_food_amount<=(max_bit/4)){
		send_to_MAX7221(set_digit_3,0);//Set tens digit
		send_to_MAX7221(set_digit_2,0); //Set ones digit
		
		//------------------------- Set tenths and hundredths digits ----------------------------
		//0 cups
		if(set_food_amount<=(max_bit/8)){
			send_to_MAX7221(set_digit_1,0);
			send_to_MAX7221(set_digit_0,0);
			meal_size = 0;
		}
		//0.5 cups
		else if(set_food_amount>(max_bit/8) && set_food_amount<=(2*max_bit/8)){
			send_to_MAX7221(set_digit_1,5);
			send_to_MAX7221(set_digit_0,0);
			meal_size = 0.5;
		}
		//-----------------------------------------------------------------------------------------
	}
	else if(set_food_amount>(max_bit/4) && set_food_amount<=(2*max_bit/4)){
		send_to_MAX7221(set_digit_3,0);//Set tens digit
		send_to_MAX7221(set_digit_2,1);//Set ones digit
		
		//------------------------- Set tenths and hundredths digits ----------------------------
		//1 cup
		if(set_food_amount>(2*max_bit/8) && set_food_amount<=(3*max_bit/8)){
			send_to_MAX7221(set_digit_1,0);
			send_to_MAX7221(set_digit_0,0);
			meal_size = 1;
		}
		//1.5 cups
		else if(set_food_amount>(3*max_bit/8) && set_food_amount<=(4*max_bit/8)){
			send_to_MAX7221(set_digit_1,5);
			send_to_MAX7221(set_digit_0,0);
			meal_size = 1.5;
		}
		//-----------------------------------------------------------------------------------------
	}
	else if(set_food_amount>(2*max_bit/4) && set_food_amount<=(3*max_bit/4)){
		send_to_MAX7221(set_digit_3,0);//Set tens digit
		send_to_MAX7221(set_digit_2,2);//Set ones digit
		
		//------------------------- Set tenths and hundredths digits ----------------------------
		//2 cups
		if(set_food_amount>(4*max_bit/8) && set_food_amount<=(5*max_bit/8)){
			send_to_MAX7221(set_digit_1,0);
			send_to_MAX7221(set_digit_0,0);
			meal_size = 2;
		}
		//2.5 cups
		else if(set_food_amount>(5*max_bit/8) && set_food_amount<=(6*max_bit/8)){
			send_to_MAX7221(set_digit_1,5);
			send_to_MAX7221(set_digit_0,0);
			meal_size = 2.5;
		}//----------------------------------------------------------------------------------------
	}
	else if(set_food_amount>(3*max_bit/4) && set_food_amount<(4*max_bit/4)){
		send_to_MAX7221(set_digit_3,0);//Set tens digit
		send_to_MAX7221(set_digit_2,3);//Set ones digit
		
		//------------------------- Set tenths and hundredths digits ----------------------------
		//3 cups
		if(set_food_amount>(6*max_bit/8) && set_food_amount<=(7*max_bit/8)){
			send_to_MAX7221(set_digit_1,0);
			send_to_MAX7221(set_digit_0,0);
			meal_size = 3;
		}
		//3.5 cups
		else if(set_food_amount>(7*max_bit/8) && set_food_amount<(max_bit)){
			send_to_MAX7221(set_digit_1,5);
			send_to_MAX7221(set_digit_0,0);
			meal_size = 3.5;
		}
		//-----------------------------------------------------------------------------------------
	}
	//4 cups
	else if(set_food_amount == max_bit){
		send_to_MAX7221(set_digit_3,0);
		send_to_MAX7221(set_digit_2,4);
		send_to_MAX7221(set_digit_1,0);
		send_to_MAX7221(set_digit_0,0);
		meal_size = 4;
	}
	return meal_size;
}

int set_hours_display (void){
	// ---------------------------------------- A/D Conversion to Set Hours Value on Display -------------------------
	//Read analog input
	ADCSRA = ADCSRA | 0b01000000;// Start conversion
	while ((ADCSRA & 0b00010000) == 0);
	
	set_time_hours = ADCH;//Read high byte of the data
	
	send_to_MAX7221(turn_on,0x01);
	wait_time = set_hours(set_time_hours);
	
	//----------------------------------------------------------------------------------------------------------------
	return wait_time;
}

float set_meal_display (void){
	//------------------------------------------ A/D Conversion to Set Food Amount on Display ------------------------
	//Read analog input
	ADCSRA = ADCSRA | 0b01000000;// Start conversion
	while ((ADCSRA & 0b00010000) == 0);
	
	set_food_amount = ADCH;//Read high byte of the data
	
	send_to_MAX7221(turn_on,0x01);
	meal_size = set_food(set_food_amount);
	
	//----------------------------------------------------------------------------------------------------------------
	return meal_size;
}

/* convert meal size from cups to a byte value */
float cups_to_byte(float meal_size){
	float meal = meal_size;
	float meal_options_cups[] = {0,0.5,1,1.5,2,2.5,3,3.5,4};
	float meal_options_byte[] = {0,100,151,170,181,187,194,200,204,206};
	int i = 0;
	bool found = 0;

	while(!found){
		if(meal == meal_options_cups[i]){
			meal_size = meal_options_byte[i+1];
			found = 1;
		}
		i++;
	}
	return(meal_size);
}

//------------------------------- FUNCTIONS FOR DIFFERENT MEAL SIZE OPTIONS ---------------------------------------------------
float fill_bowl_halfcup(float weight_sensor,float meal_size){
	float fill_amount = 0;
	float weight = weight_sensor;
	float meal = meal_size;
	float fill_needs = meal-weight;
	float meal_options [] = {0,0.5};
	int ranges [] = {0,51,151};
	int i=0;
	bool found = 0;

	while(!found){
		if(fill_needs == meal){
			fill_amount = meal_options[1];
		found = 1;}//check for max value
		else if(fill_needs>=ranges[i] && fill_needs<ranges[i+1]){
			fill_amount = meal_options[i];
		found = 1;}//sort through range of differences
		i++;
	}//end while loop
	return fill_amount;
}//end function

float fill_bowl_1cup(float weight_sensor,float meal_size){
	float fill_amount = 0;
	float weight = weight_sensor;
	float meal = meal_size;
	float fill_needs = meal-weight;
	float meal_options [] = {0,0.5,1};
	int ranges [] = {0,19,70,170};
	int i=0;
	bool found = 0;

	while(!found){
		if(fill_needs == meal){
			fill_amount = meal_options[2];
		found = 1;}//check for max value
		else if(fill_needs>=ranges[i] && fill_needs<ranges[i+1]){
			fill_amount = meal_options[i];
		found = 1;}//sort through range of differences
		i++;
	}//end while loop
	return fill_amount;
}

float fill_bowl_1andhalfcup(float weight_sensor,float meal_size){
	float fill_amount = 0;
	float weight = weight_sensor;
	float meal = meal_size;
	float fill_needs = meal-weight;
	float meal_options [] = {0,0.5,1,1.5};
	int ranges [] = {0,11,30,81,181};
	int i=0;
	bool found = 0;

	while(!found){
		if(fill_needs == meal){
			fill_amount = meal_options[3];
		found = 1;}//check for max value
		else if(fill_needs>=ranges[i] && fill_needs<ranges[i+1]){
			fill_amount = meal_options[i];
		found = 1;}//sort through range of differences
		i++;
	}//end while loop
	return fill_amount;
}

float fill_bowl_2cup(float weight_sensor,float meal_size){
	float fill_amount = 0;
	float weight = weight_sensor;
	float meal = meal_size;
	float fill_needs = meal-weight;
	float meal_options [] = {0,0.5,1,1.5,2};
	int ranges [] = {0,6,17,36,87,187};
	int i=0;
	bool found = 0;

	while(!found){
		if(fill_needs == meal){
			fill_amount = meal_options[4];
		found = 1;}//check for max value
		else if(fill_needs>=ranges[i] && fill_needs<ranges[i+1]){
			fill_amount = meal_options[i];
		found = 1;}//sort through range of differences
		i++;
	}//end while loop
	return fill_amount;
}

float fill_bowl_2andhalfcup(float weight_sensor,float meal_size){
	float fill_amount = 0;
	float weight = weight_sensor;
	float meal = meal_size;
	float fill_needs = meal-weight;
	float meal_options [] = {0,0.5,1,1.5,2,2.5};
	int ranges [] = {0,7,13,24,43,94,194};
	int i=0;
	bool found = 0;

	while(!found){
		if(fill_needs == meal){
			fill_amount = meal_options[5];
		found = 1;}//check for max value
		else if(fill_needs>=ranges[i] && fill_needs<ranges[i+1]){
			fill_amount = meal_options[i];
		found = 1;}//sort through range of differences
		i++;
	}//end while loop
	return fill_amount;
}

float fill_bowl_3cup(float weight_sensor,float meal_size){
	float fill_amount = 0;
	float weight = weight_sensor;
	float meal = meal_size;
	float fill_needs = meal-weight;
	float meal_options [] = {0,0.5,1,1.5,2,2.5,3};
	int ranges [] = {0,6,13,19,30,49,100,200};
	int i=0;
	bool found = 0;

	while(!found){
		if(fill_needs == meal){
			fill_amount = meal_options[6];
		found = 1;}//check for max value
		else if(fill_needs>=ranges[i] && fill_needs<ranges[i+1]){
			fill_amount = meal_options[i];
		found = 1;}//sort through range of differences
		i++;
	}//end while loop
	return fill_amount;
}

float fill_bowl_3andhalfcup(float weight_sensor,float meal_size){
	float fill_amount = 0;
	float weight = weight_sensor;
	float meal = meal_size;
	float fill_needs = meal-weight;
	float meal_options [] = {0,0.5,1,1.5,2,2.5,3,3.5};
	int ranges [] = {0,4,10,17,23,34,53,104,204};
	int i=0;
	bool found = 0;

	while(!found){
		if(fill_needs == meal){
			fill_amount = meal_options[7];
		found = 1;}//check for max value
		else if(fill_needs>=ranges[i] && fill_needs<ranges[i+1]){
			fill_amount = meal_options[i];
		found = 1;}//sort through range of differences
		i++;
	}//end while loop
	return fill_amount;
}

float fill_bowl_4cup(float weight_sensor,float meal_size){
	float fill_amount = 0;
	float weight = weight_sensor;
	float meal = meal_size;
	float fill_needs = meal-weight;
	float meal_options [] = {0,0.5,1,1.5,2,2.5,3,3.5,4};
	int ranges [] = {0,2,6,12,19,25,36,55,106,206};
	int i=0;
	bool found = 0;

	while(!found){
		if(fill_needs == meal){
			fill_amount = meal_options[8];
		found = 1;}//check for max value
		else if(fill_needs>=ranges[i] && fill_needs<ranges[i+1]){
			fill_amount = meal_options[i];
		found = 1;}//sort through range of differences
		i++;
	}//end while loop
	return fill_amount;
}

//----------------------------------------- STEPPER MOTOR FUNCTIONS ---------------------------------------------------------
void step_CCW()
{
	// This function advances the motor counter-clockwise one step.  Follow the full-wave stepping table in Stepper Motor Control.ppt for MEMS 1049 course. phase1a = PORTD_7, phase1b = PORTD_6
	// phase2a = PORTD_5, phase2b = PORTD_4

	switch (phase_step) {
		case 1:
		// step to 2
		PORTD = 0b00010000;
		phase_step = 2;
		break;
		case 2:
		// step to 3
		PORTD = 0b01000000;
		phase_step = 3;
		break;
		case 3:
		// step to 4;
		PORTD = 0b00100000;
		phase_step = 4;
		break;
		case 4:
		// step to 1;
		PORTD = 0b10000000;
		phase_step = 1;
	break;  }
}// end step_CCW

void step_CW() {
	// This function advances the motor clockwise one step.  Follow the full-wave stepping table in Stepper Motor Control.ppt for MEMS 1049 course. Our stepper motor phases are assigned to Port pins as follows: phase1a = PORTD_7, phase1b = PORTD_6, phase2a = PORTD_5, phase2b = PORTD_4

	switch (phase_step) {
		case 1:
		// step to 4
		PORTD = 0b00100000;
		phase_step = 4;
		break;
		case 2:
		// step to 1
		PORTD = 0b10000000;
		phase_step = 1;
		break;
		case 3:
		// step to 2;
		PORTD = 0b00010000;
		phase_step = 2;
		break;
		case 4:
		// step to 3;
		PORTD = 0b01000000;
		phase_step = 3;
	break;  }
}// end step_CW

/*Function to open door*/
void door_open(void){
	// Step forward 417 steps or 750deg
	for (int i=0; i < 119; i++)
	{
		step_CW(); //step_CW()
		wait(50,1); // delay so that motor doesn't skip steps
	}
}

/*Function to close door*/
void door_close(void){
	// Step backward 417 steps or 750deg
	for (int i=0; i < 119; i++)
	{
		step_CCW(); // step_CCW()
		wait(50,1); // delay so that motor doesn't skip steps
	}
}
/*Function for full door operation*/
void door(int disp_time){
	//if dispensing time = 0 (food bowl is full), don't open door
	if(disp_time != 0){
		door_open();
		wait(disp_time,2);
		door_close();
	}
}

/*
ISR(INT0_vect){
	
	//------------------------------------------- IF PROXIMITY SENSOR DETECTS EMPTY RESERVE ------------------------------------
	unsigned char save_PORTB,save_PORTD,save_ADMUX;
	save_PORTB = PORTB;
	save_PORTD = PORTD;
	save_ADMUX = ADMUX;
	
	
	unsigned char F = 0b01000111;
	unsigned char I = 0b00000110;
	unsigned char L = 0b00001110;
	
	
	bool full = 0;
	
	//reinitialize AD conversion for proximity sensor
	ADMUX = 0b00100100;
	
	//start AD conversion for proximity sensor
	ADCSRA = ADCSRA | 0b01000000;// Start conversion
	while ((ADCSRA & 0b00010000) == 0);
	proximity = ADCH;//Read high byte of the data
	
	while(!full){
		if(proximity <= 0.5*max_bit){
			full = 1;
		}
		else if(proximity > 0.5*max_bit){
			
			send_to_MAX7221(set_digit_3,0);
			send_to_MAX7221(set_digit_2,0);
			send_to_MAX7221(set_digit_1,0);
			send_to_MAX7221(set_digit_0,0);
			
			send_to_MAX7221(set_digit_3,F);
			send_to_MAX7221(set_digit_2,I);
			send_to_MAX7221(set_digit_1,L);
			send_to_MAX7221(set_digit_0,L);
		}
	}
	
	
	PORTB = save_PORTB;
	PORTD = save_PORTD;
	ADMUX = save_ADMUX;
	EIFR = 0b00000001;  // Clear the INT0 interrupt flag in case interrupt was retriggered
}*/

ISR(INT0_vect){
	
	unsigned char save_PORTB;
	save_PORTB = PORTB;
	/*
	unsigned char decode = 0b00001001;
	unsigned char digits = 0x00;
	
	send_to_MAX7221(decode,digits);
	
	unsigned char S = 0b01011011;
	unsigned char E = 0b01000111;
	unsigned char t = 0b00001111;
	*/
	
	//------------------------------------- PRESS BUTTON TO CONFIRM SETTING CHANGE --------------------------------------
	found_wait = 0;
	found_meal = 0;
	found_exit = 0;
	
	send_to_MAX7221(set_digit_3,0);
	send_to_MAX7221(set_digit_2,0);
	send_to_MAX7221(set_digit_1,0);
	send_to_MAX7221(set_digit_0,0);
	
	PORTB = save_PORTB;
	EIFR = 0b00000001;  // Clear the INT0 interrupt flag in case interrupt was retriggered
	/*
	digits = 0b00001111;
	send_to_MAX7221(decode,digits);*/
}