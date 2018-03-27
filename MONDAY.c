/*
 * TIMER1 Servo.c
 *
 * Created: 3/3/2018 2:05:36 PM
 * Author : Carl
*/

#define F_CPU 14745600 //required definition for <util/delay.h>

//libraries
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

//clock frequencies at various prescalers
#define fclk_1 14745600
#define fclk_8 1843200
#define fclk_64 230400
#define fclk_128 115200
#define fclk_256 57600
#define fclk_1024 14400

#define servo_period 8e-3 //8ms pwm period
#define min_duty_cycle 0.07 //experimentally determined 
#define max_duty_cycle 0.3 //experimentally determined

volatile short ext_flag = 0;

ISR(INT2_vect){
	ext_flag = 1;
}
//usart function declarations + file pointers
int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);
FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE mystdin = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

ISR(INT0_vect){
	OCR1B = (unsigned short)(min_duty_cycle*OCR1A);
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
	printf("\nUSART system booted\n");
}

void timer1_servo_cfg(){
		TCCR1A |= (1<<COM1B1);
		//on compare match, clear OC1B on upcount, set OC1B on downcount
		
		//configure for phase correct pwm TOP at OCR1A
		TCCR1A |= (1<<WGM10);
		TCCR1B |= (1<<WGM13);
		OCR1A = (unsigned int)(fclk_1*servo_period/2.0); //set TOP value
		TCCR1B |= (1<<CS10); //no prescaler
		TCNT0 = 0;
}

void interrupt_cfg(){
	EIMSK |= (1<<INT2)|(1<<INT0); //INT2 = PB2
	EICRA |= (1<<ISC21)|(1<<ISC20); //rising edge trigger
	EICRA |= (1<<ISC00)|(1<<ISC01); //rising edge trigger
}

int main(void){
	init_uart();
	timer1_servo_cfg();
	interrupt_cfg();
	sei();
	DDRD |= (1<<PD4); //set OC1B to output 
	PORTB &= ~(1<<PB2);
	DDRB |= (1<<PB2);
	const float duty_per_deg = ((max_duty_cycle - min_duty_cycle)/180.0);
	unsigned short delta_theta = 5; //CHANGE THIS TO CHANGE THE INCREMENT ANGLE
	unsigned short cnts_per_deg = (unsigned short)(duty_per_deg*OCR1A);
	unsigned short min_pos_cnts = (unsigned short)(min_duty_cycle*OCR1A);
	unsigned short max_pos_cnts = (unsigned short)(max_duty_cycle*OCR1A);
	unsigned short cnts_per_position = (unsigned short)(cnts_per_deg*delta_theta);
	OCR1B = min_pos_cnts; //initialize motor control to position 0.
    while (1) 
    {
		if(ext_flag){
			ext_flag = 0; //clear external interrupt flag
			OCR1B += cnts_per_position;
			if(OCR1B > max_pos_cnts){
				OCR1B = min_pos_cnts;	
			}
		}	
    }//end of while	


}//end of main

	
