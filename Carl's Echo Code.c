/*
 * Echo_attempt_1.c
 *
 * Created: 2/12/2018 2:16:36 PM
 *  Author: Carl Mattatall
 */ 

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

const float trigger_pulse_length = 10e-6;
//minimum voltage pulse of 10 microseconds to sonic ranger trigger pin.

const float clk_frequency = 14.7456e6;

volatile unsigned int time_elapsed = 0;

volatile unsigned short trigger_flag = 0;

volatile unsigned int echo_pulse_length = 0;

volatile unsigned int pulse_count_length = 0;

volatile unsigned short overflow_counts = 0;

volatile unsigned short usart_flag = 0;




ISR(INT0_vect){
	trigger_flag = 1;
	//this is the interrupt to actually initiate sensing of an object
	//we only care about rising edge trigger from the button press.
}


ISR(TIMER1_COMPA_vect){
	//this triggers every 10 microseconds
	
	//we use this to track the 10 microsecond trigger pulse
	time_elapsed += 10;
}

ISR(TIMER1_OVF_vect){
	
	overflow_counts += 1;
	
	TCNT1 = 0;
	//probably not necessary
}


ISR(TIMER1_CAPT_vect){
	
	//if this ISR has triggered, the timer1-compa-vect mask is disabled.
	
	
	if( TCCR1B && (1<<ICES1) == (1<<ICES1)){ //if we are detecting rising edges 
		//if ICES1 bit = 1, event occurs on rising edge
		//if ICES1 bit = 0, event occurs on falling edge
		
		TCNT1 = 0;
		//clear the timer1 counter
		
		//*******TEST LED*****
		PORTD |= (1<<PD5);
		PORTD |= (1<<PD3);
		//if this LED toggles when we press the button
		//if means the ultrasonic ranger is getting a signal back
		 
		ICR1 = 0;
		
		//TCCR1B &= ~(1<<ICES1);
		//set to falling edge trigger
	}
	
	
	if(TCCR1B || (~(1<<ICES1)) == ~(1<<ICES1)){
	 //(we are detecting falling edges)
		pulse_count_length = ICR1; 
		
		//*******TEST LED*******
		PORTD &= ~(1<<PD5); 
		PORTD &= ~(1<<PD3);
		
		//if this LED toggles we have successfully found the falling pulse edge
		
		TIMSK1 &= ~(1<<ICIE1);
		//disable input capture mode because we have 
		//detected the falling edge of the pulse.
		
		//TCCR1B |= (1<<ICES1);
		//set to rising edge trigger
		
		printf("The value of pulse_count_length is %d \n ", pulse_count_length);
	}
	
	
}




//**********************************USART STUFF********************************

int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);
FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE mystdin = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

int uart_putchar(char c, FILE *stream)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
	return 0;
}


int uart_getchar(FILE *stream)
{
	/* Wait until data exists. */
	loop_until_bit_is_set(UCSR0A, RXC0);
	return UDR0;
}


void init_uart(void)
{
	UCSR0B = (1<<RXEN0) | (1<<TXEN0);
	UBRR0 = 95;//7; // configures a baud rate of 115200
	stdout = &mystdout;
	stdin = &mystdin;
	printf("USART system booted\n");
}
//**************************END OF USART STUFF********************************
	
void configure_ports(){
	DDRB |= (1<<PB1); //PB1 is connected to sonic ranger device's trigger pin
	DDRD |= (1<<PD5);
	DDRD |= (1<<PD3);
	PORTB = 0; //write all B pins low
	PORTD = 0; //write all D pins low (used for test LEDs)
}	
	

void configure_timer(){
	
	TCCR1B |= (1<<WGM12); //CTC mode with top value OCR1A
	TCCR1B |= (1<<CS11);  //prescaling factor of 8
	//JF WANTS PRESCALING OF 8 BUT THAT DOESNT MAKE ANY SENSE
	//BECAUSE 147 COUNTS HAPPILY FITS INSIDE A 16 BIT TIMER.
	
	TCCR1B |= (1<<ICES1);
	//set input capture edge mode to rising edge trigger
	
	//TCCR1B |= (1<<ICNC1);
	//enable input capture noise canceller
	//not sure if this is needed/
	
	OCR1A = (int)(clk_frequency*trigger_pulse_length/8);
	//14.7456e6 * 10e-6 = 147.456 counts per 10 microsec.
	//  18.375 ~ 18 counts
	//this introduces a bit of innaccuracy 
	//unfortunately OCR1A only takes numerical integers (not the data type)
	
	
	TIMSK1 |= (1<<OCIE1A)|(1<<TOIE1);
	//enable mask bit for output compare interrupt 1A
	//enable timer1 overflow ISR
	
	//WE DO NOT ENABLE INPUT CAPTURE MODE HERE,
	//THAT ONLY OCCURS AFTER WE HAVE SENT THE 10MICROSEC
	//PULSE TO THE SONIC RANGER WHEN THE FUNCTION TRIGGER HAS
	//BEEN CALLED!!!!!
	
}

void configure_external_interrupts(){
	
	EICRA |= (1<<ISC01)|(1<<ISC00); 
	//set INT0_vect to only trigger on rising edge
	//This will be set off by touching a wire to 
	//the "button" of pin D2 
	
	EIMSK |= (1<<INT0);
	//enable INT0 in the external interrupt mask register
}





void trigger(){
	
	if(trigger_flag == 1){
		trigger_flag = 0;
		TIFR1 = 0x00;
		
		//clear any existing timer1 flags.
		//This is in case we want to reset by pressing a button
		//in the middle of waiting for the returned echo pulse.
		
		TCNT1 = 0;
		//set timer count at 0
		
		TIMSK1 |= (1<<OCIE1A);
		//enable the timer mask to allow the compare ISR to start counting
		
		PORTB |= (1<<PB1); //write pin B1 high
		time_elapsed = 0; //clear timer
		while(time_elapsed < 10){} //10 microsecond delay
		PORTB &= ~(1<<PB1); //write PB1 low
		
		printf("The value of time_elapsed is %d \n ", time_elapsed);
		TCCR1B |= (1<<ICES1);
		//set input capture ISR trigger type to rising edge
		
		TIMSK1 |= (1<<ICIE1);
		//enable input capture mode
		//because we will be looking for the rising edge
		//of the returning pulse

		TIMSK1 &= ~(1<<OCIE1A);
		//disable the 10ms counter interrupt because timer1-capt-vect
		//directly puts the TCNT1 value into ICR1 upon its execution.
		//all we need to account for is timer1 overflow
			
	}
		
}


int main(void)
{
	init_uart(); 
	configure_ports();
	configure_timer();
	configure_external_interrupts();
	sei();
	

	
    while(1)
    {
		trigger();
		
		
	}
	
	
}













