/*
 * Lab 7.cpp
 *
 * Created: 3/17/2022 2:18:28 PM
 * Author : cfull
 */ 

#include <avr/io.h>

// Functions
void step_CW( void );
void step_CCW( void );
void wait(volatile int multiple, volatile char time_choice);
void delay_T_msec_timer0(char choice);
void waggle(void);

//** GLOBAL VARIABLES **
int phase_step = 1; // We will use wave stepping, so the steps are numbered 1-4, then will repeat.  

int main(void)
{
   // Setup
	DDRD = 0xF0; // Sets pins 4, 5, 6, and 7 output, and 	pins 0, 1, 2 and 3 to input. Pins 4-7 will be used for	stepper motor control
	DDRC = 0x00; //Set PC0 as input - this is the switch
	DDRB = 1<<DDB1 | 1<<DDB0; //Set PB0 and PB1 to output - these are the LEDs
	PORTB = 0b00000011;
	
   while(1)
   {
	//Wait for switch to be pressed
	if(!(PINC & 0b00000001)){
		
		//Rotate 180 degrees CW in 5 seconds
			//Each step = 7.5 degrees ---> 180 deg = 24 steps
		for (int i=24;i>0;i--){
			PORTB = ~0b00000010; //Turn on green LED (PB0) while motor turns CW
			step_CW();
			wait(104,2); // 5 sec/24 steps =208.333 ms/step
			
			//If switch is pressed during this process:
			if(!(PINC & 0b00000001)){
				waggle();
			}
		}
		
		//Pause for 1 second
		wait(500,2);
		
		//Rotate 360 degrees CCW in 5 seconds
			//360 deg ---> 48 steps
		for (int i=48;i>0;i--)
		{
			PORTB = ~0b00000001; //Turn on red LED (PB1) while motor turns CCW
			step_CCW();
			wait(52,2); // 5 sec/48 steps = 104.17 ms/step
			
			//If switch is pressed during this process:
			if(!(PINC & 0b00000001)){
				waggle();
			}
		}
	PORTB = ~0b00000000;
	}
			
  
   } // end main while

}

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

void waggle(void){
	
	for (int j=3;j>0;j--){
		for (int i=6;i>0;i--){
			PORTB = ~0b00000001; //Turn on red LED (PB1) while motor turns CCW
			step_CCW();
			wait(41,2);
		}
		for (int i=6;i>0;i--){
			PORTB = ~0b00000010; //Turn on green LED (PB0) while motor turns CW
			step_CW();
			wait(41,2);
		}
		
	}
}//end waggle()

