


#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>







const float fclk = 14.7456e10;
const float trigger_pulse_length = 10e-6;


unsigned int sonic_pulse_counts = 0;
unsigned short echo_flag = 0;

float object_distance = 0.0;





ISR(INT0_vect){
	
	echo_flag = 1;
	printf("USART IS WORKING");
}


ISR(TIMER0_COMPA_vect){
	
	PORTB &= ~(1<<PB1); //turn off trigger pin input
	TCCR0B &= ~(1<<CS00); //turn off clock
	TIMSK0  &= ~(1<<OCIE0A); //disable the mask for this interrupt
}


ISR(TIMER1_CAPT_vect){
	
	if(ICR1 == 0){
		
		TCCR1B &= ~(1<<ICES1); //set edge capture trigger to falling edge
		
		TCCR1B |= (1<<CS01); //turn on clock at 8 prescaler
		
		
	}
	
	
	else{
		
		TCCR1B &= ~(1<<CS01); //turn off clock from 8 prescaler
		
		TCNT1 = 0;
		
		TCCR1B |= (1<<ICES1); //set back to rising edge ISR capture trigger
		
		TIMSK1 &= ~(1<<ICIE1); //disable future input capture interrupts.
		
	}
	
	
	
	
	
}

void configure_timers(){
	
	
	TCCR0A |= (1<< WGM01); //CTC mode
	OCR0A = (int)fclk*trigger_pulse_length; //this is approximately 147 register counts
	
	TCCR1B |= (1<<ICES1);
	
	ICR1 = 0;
	TCNT1 = 0;
	
	
	
}


void configure_ports(){
	
	DDRD = 0;
	
	DDRB |= (1<<PB1);
	
	//****TEST LEDS GO BELOW****
	
	
	
	
}

void configure_interrupts(){
	
	EICRA |= (1<<ISC00)|(1<<ISC01);
	//rising edge only
	
	EIMSK |= (1<<INT0);
	//enable the mask
	
}





//*********************USART STUFF*******************************************
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

//**************************************END OF USART STUFF*********************






int echo_trigger(){
	
	PORTB |= (1<<PB1);
	TIMSK0 |= (1<<OCIE0A); //enable the timer0 interrupt compare ISR
	TCCR0B |= (1<<CS00); //enable timer0 clock (no prescalers)
	ICR1 = 0;
	TCNT1 = 0;
	TIMSK1 |= (1<<ICIE1);
	while(ICR1 == 0){}
	
	return ICR1;
}



int main(void){
	
	configure_ports();
	configure_timers();
	configure_interrupts();
	sei();
	
	printf("USART system booted\n");
	//convert sonic_pulse_counts to object_distance
	
	

	while(1){
		
		sonic_pulse_counts = echo_trigger();
		
		printf("The value of sonic_pulse_counts is %d \n ", sonic_pulse_counts);
		ICR1 = 0;
		
		
	}
}