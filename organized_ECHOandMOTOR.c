/*
 * TIMER0 Phase Correct.c
 *
 * Created: 3/10/2018 1:38:58 PM
 * Author : Carl
 */ 
#define F_CPU 14745600 //required definition for <util/delay.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

//clock frequencies at various prescalers
const float fclk_1 = 14745600;
const float fclk_8 = 1843200;
const float fclk_256 = 57600;
const float fclk_1024 = 14400;

const float servo_period = 35e-3; //35ms
//35 ms is longest time period that will fit in 2*256 cnts at 1024 prescaler.

//PWM pulse lengths (in seconds) for min and max servo motor positions.
const float min_pos_t = 0.6e-3; //1ms
const float max_pos_t = 2.4e-3; //4.8ms

unsigned short motor_angle = 0;
unsigned short echo_flag = 0;
unsigned int pulse_counts = 0;
unsigned int object_distance = 0; //units in cm

//these need to be type int because they measure TCNT1 which can go up to 16 bit.
volatile unsigned int Rising_Edge_cnt = 0;
volatile unsigned int Falling_Edge_cnt = 0;

//usart function declarations + file pointers
int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);
FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE mystdin = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);


ISR(TIMER1_CAPT_vect){
	//Capture the value of the ICR1 when the rising edge is found
	//Capture the value of the ICR1 when the falling edge is found
	if (TCCR1B & (1<<ICES1)){
		Rising_Edge_cnt = ICR1;
		TCCR1B &= ~(1<<ICES1); //set to falling edge ISR trigger
	}
	else{
		Falling_Edge_cnt = ICR1;
		TCCR1B |= (1<<ICES1); //set to rising edge ISR trigger
	}
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

void ports_config(){
	//****PORTS FOR SERVO MOTOR****
	DDRB |= (1<<PB4)|(1<<PB3); 
	//OC0B = PB4, OC0A = PB3

	//*****PORTS FOR ECHOLOCATOR******
	DDRB |= (1<<PB1);

	//******PORTS FOR AUDIO SPEAKER******
}

void timer0_config(){
	//TIMER0 IS USED FOR THE SERVO MOTOR
	
	TCCR0A |= (1<<COM0B1);//clear OC0B on up-count, set OC0B on down-count
	TCCR0A |= (1<<WGM00)|(1<<WGM02); //Phase correct pwm with top at OCR0A
	TCCR0B |= (1<<CS00)|(1<<CS02); //1024 prescaler
	OCR0A = (short)(fclk_1024*servo_period/2);
	
	//divide OCR0A by 2 because phase correct mode.
	// TCNT0 counts up to TOP (OCR0A in this case)...
	//and then counts down to BOTTOM (0x00);
	
	TCNT0 = 0; //clear timer just in case
}

void timer1_config(){
	//TIMER1 IS USED FOR THE ECHOLOCATOR

	TCCR1B = (1<<CS11)|(1<<CS10); //start TCNT1 counting at 64 prescaler
	TCCR1B |= (1<<ICES1); //set rising edge trigger mode on the input capture ISR
	TIMSK1 |= (1<<ICIE1); //enable input capture interrupt
	TCNT1 = 0; 
}

void timer2_config(){
	//TIMER2 IS USED FOR THE AUDIO SPEAKER
	
	//we want 50% duty cycle always so that means OCR0B = OCR0A/2
	//use fast pwm with prescaler of 256 to make the frequency range
	//of 260hz to 493 hz fit inside the 8bit timer2.
}


void trigger_echo(){
	PORTB |= (1<<PB1); //write pin B1 high, wait 10microsec, write PB1 low.
	_delay_us(10);
	PORTB &= ~(1<<PB1);
	TCCR1B |= (1<<ICES1); //set edge detection to rising edge 
	TCNT1 = 0; //clear timer
}

int main(void){
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
	
    while (1){
		//clear variables.
		motor_angle = 0;
		pulse_counts = 0;
		object_distance = 0;
		
		
		for(unsigned int x = (int)(min_duty_cyc*OCR0A); x <= (int)(max_duty_cyc*OCR0A); x += (int)(duty_per_deg*delta_theta)){
			OCR0B = x;
			motor_angle += delta_theta; //motor angle type = unsigned short
			printf("motor angle is %u\n", motor_angle);
			//motor angle is passed to USART
			//object distance is also passed via USART
			
			trigger_echo();
			pulse_counts = Falling_Edge_cnt -Rising_Edge_cnt ;
			object_distance = (int)((pulse_counts*4)/58); 
			//object distance type = unsigned int
			
			if(object_distance > 420){
				printf("Object out of range\n");
				//420 centimeters is 4m with 5% error in measurement
			}
			else{
				printf("distance = %u\n", object_distance);
			}
			_delay_ms(500);
		}
    }
}



