
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>


const float fclk = 14.7456e10;
const float trigger_pulse_length = 10e-6;


volatile unsigned int sonic_pulse_counts = 0;

volatile unsigned short echo_flag = 0;
volatile unsigned short microsecs_elapsed = 0;



float object_distance = 0.0; 

//**** USART FILE IMPORTS AND FUNCTION DECLARATIONS***
int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);
FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE mystdin = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);




ISR(INT0_vect){
	printf("The External ISR Works \n ");
	echo_flag = 1;
}

ISR(TIMER0_COMPA_vect){
	TIMSK0 &= ~(1<<OCIE0A); //disable interrupt mask
	microsecs_elapsed += 10; //increment 
	printf("The 10microsec timer works\n"); //feedback
}



ISR(TIMER1_CAPT_vect){
	// printf("We have entered the input capture ISR\n"); 
	
	//whenever this ISR occurs, the value of TCNT1 is stored in ICR1.
	//We are using this to measure the length of the sonic pulse (in clock counts)
	//The clock counts will then be converted to distance later
	
	
	 if(ICR1 == 0) {
	 	// printf("THE RISING EDGE INPUT CAPTURE WORKS \n");
	
	 	TCCR1B &= ~(1<<ICES1); //set to falling edge trigger
		
		TCCR1B |= (1<<CS11); //start timer at 8 prescaler
	}

	else{ 
		TCCR1B &= ~(1<<CS11); //stop timer1 clock 
		
		printf("THE FALLING EDGE INPUT CAPTURE WORKS \n"); //feedback 
		printf("The Value of sonic_pulse_counts is: %d \n ", sonic_pulse_counts);
		
		TCCR1B |= (1<<ICES1); //set detection mode back to rising edge 
		TIMSK1 &= ~(1<<ICIE1); //disable future input capture interrupts
		
	}
	
	
}





void configure_timers(){
	
	//***TIMER0****
	TCCR0A |= (1<<WGM01); //CTC Mode
	OCR0A = (int)fclk*trigger_pulse_length; //147 counts per 10 microsec
	TCCR0B |= (1<<CS00); //turn on clock @ prescaler of 0
	
	
	//***TIMER1****
	TCCR1B |= (1<<ICES1); //rising edge trigger for input capture (PD6).
	
	
	
	
	
	
}

void configure_ports(){	
	DDRD = 0;
	DDRB |= (1<<PB1);
	//****TEST LEDS GO BELOW***
}

void configure_external_interrupts(){
	EICRA |= (1<<ISC00)|(1<<ISC01);
	//rising edge only on INT0 (PD2)
	
	EIMSK |= (1<<INT0);
	//enable the mask
}



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
	printf("\n USART system booted\n");
}




void send_trigger_pulse(){
	TIMSK0 |= (1<<OCIE0A); //enable the TIMER0 compare ISR 
	//(The ISR that increments microsec_elapsed by 10 every 10 microseconds)
	
	PORTB |= (1<<PB1); //write trigger pin connection high
	while(microsecs_elapsed < 10){} //10 microsec delay
	PORTB &= ~(1<<PB1); //write trigger pin connection low
}



int measure_sonic_pulse(){
	TCCR1B |= (1<<ICES1); //rising edge trigger mode for input capture ISR
	TIMSK1 |= (1<<ICIE1); //enable the mask for input capture ISR
	
	TCNT1 = 0; //clear timer
	
	while(ICR1 == 0){}
	//we wait until the input capture ISR changes the value of ICR1 
	//to something other than its initialized value of 0.
	//Since the value of ICR1 is only changed at the falling edge
	//of input capture, this means we have sucessfully found...
	//the length of the sound wave in clock counts.
	return ICR1;
}





int main(void){
	
	configure_ports();
	configure_timers();
	configure_external_interrupts();
	init_uart();
	sei();
	
	
	//convert sonic_pulse_counts to object_distance
	
	

	while(1){
		if(echo_flag){
			
//			printf("The echo flag if statement works \n");
			echo_flag = 0;
			send_trigger_pulse();
			sonic_pulse_counts = measure_sonic_pulse();	
			ICR1 = 0; 
			//measure_sonic_pulse returns ICR1 
		}
	
	
	}				
}
