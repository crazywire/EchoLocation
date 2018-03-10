/*
 * TIMER0 Phase Correct.c
 *
 * Created: 3/10/2018 1:38:58 PM
 * Author : Carl
 */ 
#define F_CPU 14745600

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include <util/delay.h>
#include <stdio.h>



const float fclk_1 = 14745600;
const float fclk_8 = 1843200;
const float fclk_256 = 57600;
const float fclk_1024 = 14400;

const float servo_period = 35e-3; //35ms
//35 ms is longest time period that will fit in 2*256 cnts at 1024 prescaler.

const float trigger_pulse_length = 10e-5; //10microsec trigger pulse 

//PWM pulse lengths (in seconds) for min and max servo motor positions.
const float min_pos_t = 0.6e-3; //1ms
const float max_pos_t = 2.4e-3; //4.8ms


unsigned short motor_angle = 0;

//these need to be type int because they measure TCNT1 which can go up to 16 bit.
volatile unsigned int Rising_Edge_time = 0; 
volatile unsigned int Falling_Edge_time = 0;
volatile unsigned int time_elapsed = 0;




unsigned int object_distance = 0; //units in cm

int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);






ISR(TIMER1_CAPT_vect){
	//Capture the value of the ICR1 when the rising edge is found
	//Capture the value of the ICR1 when the falling edge is found
	if (TCCR1B & (1<<ICES1)){
		Rising_Edge_time = ICR1;
		
												printf("rising edge works\n"); //feedback
		
		TCCR1B &= ~(1<<ICES1); //Select the falling edge
		}else{
		Falling_Edge_time = ICR1;
												printf("falling edge works\n"); //feedback
		
		TCCR1B |= (1<<ICES1); //Select the rising edge
	}
	
}





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
	printf("\nUSART system booted\n");
}

void ports_config(){
	//****PORTS FOR SERVO MOTOR****
	DDRB |= (1<<PB4)|(1<<PB3); 
	//OC0B = PB4, OC0A = PB3

	//*****PORTS FOR ECHOLOCATOR******
	
	
	DDRB |= (1<<PB1);
	//******PORTS FOR AUDIO SPEAKER******
	
	
}


void interrupt_cfgs(){

	TIMSK1 = (1<<ICIE1); //enable input capture using timer 1.
	
	sei();
	
	
}


void timer0_config(){
	//TIMER0 IS USED FOR THE SERVO MOTOR
	
	TCCR0A |= (1<<COM0B1);
	//clear OC0B on up-count, set OC0B on down-count
	
	TCCR0A |= (1<<WGM00)|(1<<WGM02);
	//Phase correct pwm with top at OCR0A
	
	TCCR0B |= (1<<CS00)|(1<<CS02); //1024 prescaler
	
	OCR0A = (short)(fclk_1024*servo_period/2);
	//divide by 2 because phase correct mode.
	// TCNT0 counts up to TOP (OCR0A in this case)...
	//and then counts down to BOTTOM (0x00);
	
	TCNT0 = 0; //clear timer just in case
	
	
	
}


void timer1_config(){
	//TIMER1 IS USED FOR THE ECHOLOCATOR

	TCCR1A =0;
	TCCR1B =(1<<CS11) |(1<<CS10)|(1<<ICES1); 
	//start timer and capture the rising edge
	
	TIMSK1 |= (1<<ICIE1); //enable input capture interrupts
	
}
void timer2_config(){
	//TIMER2 IS USED FOR THE AUDIO SPEAKER
	
	
	
	
}


void trigger_echo(){
	PORTB |= (1<<PB1); //write pin B1 high, wait 10microsec, write PB1 low.
	_delay_us(10);
	PORTB &= ~(1<<PB1);
	TCCR1B |= (1<<ICES1); //set edge detection to rising edge 
	TCNT1 = 0;
}

	
	
	





int main(void)
{
	init_uart();
    ports_config();
	timer0_config();
	timer1_config();
	timer2_config();
	sei();
	

	

	
	const float min_duty_cyc = min_pos_t/servo_period;
	const float max_duty_cyc = max_pos_t/servo_period;
	const float duty_per_deg = (max_duty_cyc - min_duty_cyc / 180);
	//divide by 180 because 180 degrees of rotation for the motor.
	
	unsigned short delta_theta = 15; 
	//this variable controls the angle increment when the motor rotates	
	


	
	
	
	
    while (1) 
    {
		//clear variables.
		motor_angle = 0;
		time_elapsed = 0;
		object_distance = 0;
		
		
		for(unsigned int x = (int)(min_duty_cyc*OCR0A); x <= (int)(max_duty_cyc*OCR0A); x += (int)(duty_per_deg*delta_theta))
		{
			
			OCR0B = x;
			motor_angle += delta_theta;
			//motor angle is the variable that is passed via USART
			//object distance is also passed via USART
			trigger_echo(); //send the 10microsec pulse to the sonic ranger
			time_elapsed = Falling_Edge_time - Rising_Edge_time; 
			object_distance = (int)((time_elapsed * 4 )/58); 
			printf("Object distance is %u cm. \n", object_distance);
			_delay_ms(500);
			
		}
		
    }
}



