#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>


const float fclk = 14.7456e6;
const float trigger_pulse_length = 10e-6;


volatile uint16_t Rising_E_time=0, Falling_E_time=0, time_elapsed;

volatile unsigned short echo_flag = 0;

unsigned int object_distance = 0;

//**** USART FILE IMPORTS AND FUNCTION DECLARATIONS***
int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);
FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE mystdin = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);




ISR(INT0_vect){
	echo_flag = 1;
}

ISR(TIMER1_CAPT_vect){
	//Capture the value of the ICR1 when the rising edge is found
	//Capture the value of the ICR1 when the falling edge is found
	if (TCCR1B & (1<<ICES1)){
		Rising_E_time = ICR1;
		TCCR1B &= ~(1<<ICES1); //Select the falling edge
	}else{
		Falling_E_time = ICR1;
		TCCR1B |= (1<<ICES1); //Select the rising edge
	}
	
}

void configure_timers(){
	TCCR1A =0;
	TCCR1B =(1<<CS11) |(1<<CS10)|(1<<ICES1); //start timer and capture the rising edge
}

void configure_ports(){	
	DDRD = 0;
	DDRB |= (1<<PB1);
}

void configure_interrupts(){
	EICRA |= (1<<ISC00)|(1<<ISC01);//rising edge only on INT0 (PD2)
	EIMSK |= (1<<INT0);//enable external interrupt
	TIMSK1 = (1<<ICIE1);
	sei();
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

//*****************MAIN METHOD**************************

int main(void){
	
	configure_ports();
	configure_timers();
	configure_interrupts();
	init_uart();
		
	
	while(1){
		if(echo_flag){
			echo_flag = 0;
			PORTB |= (1<<PB1); // start trigger pulse(ISR turns this pin off);
			_delay_us(10);
			PORTB &= ~(1<<PB1);
			time_elapsed = Falling_E_time-Rising_E_time;
			object_distance= (int)((time_elapsed*4)/58);
			printf("The object distance is %d cm \n", object_distance);
		}
	}				
}
