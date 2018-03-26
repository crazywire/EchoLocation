/*
 * integrated audio, speaker, motor (with calibration)
 *
 * Created: 3/13/2018 
 * Author : GROUP 20
 */ 
 
#define F_CPU 14745600 //required definition for <util/delay.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#define fclk_8 F_CPU/8
#define fclk_64 F_CPU/64
#define fclk_128 F_CPU/128
#define fclk_256 F_CPU/256
#define fclk_1024 F_CPU/

#define spd_sound 34300 //units in cm per sec
#define distance_factor_8 spd_sound/(2*fclk_8) 
//^converts clock counts to cm at 8 prescaler
#define servo_period 0.02 //20ms
#define min_duty_cyc 0.027 //experimentally determined value
#define max_duty_cyc 0.121 //experimentally determined value
#define duty_per_degree ((max_duty_cyc-min_duty_cyc)/180)
#define max_distance 400 //units in cm
short motor_angle = -90;
unsigned int object_distance[] = {0,0,0}; //units in cm
volatile short reset_flag = 0;

//FUNCTION PROTOTYPES
int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);
void init_uart(void);
void timer1_echo_config();
void timer1_servo_config();
void timer2_speaker_config();
void ports_config();
void audio_feedback(unsigned int distance);
void rotate_motor(unsigned short angle);
int measure_distance();
void reset_config();

FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE mystdin = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);


ISR(TIMER1_CAPT_vect){
	//Capture the value of the ICR1 when the falling edge is found
	if (TCCR1B & (1<<ICES1)){
		TCNT1 = 0;
		ICR1 = 0;
		TCCR1B &= ~(1<<ICES1); //set to falling edge ISR trigger
	}
	else{
		TCCR1B |= (1<<ICES1); //set to rising edge ISR trigger
	}
}


ISR(INT0_vect){
	reset_flag = 1; //set the reset flag
}


int main(void){
	init_uart();
	ports_config();
	timer1_echo_config();
	object_distance[0] = measure_distance(); //measure the initial distance
	timer1_servo_config(); 
	//^ this sets the initial value of OCR1A for the motor period.

	timer2_speaker_config(); 
	reset_config(); 
	sei();
	
	unsigned short delta_theta = 10; //change this to change the angle increment
	
	OCR1B = (unsigned int)(min_duty_cyc*OCR1A); //set motor to min position

																										
	while (1){
		if(reset_flag){
			timer1_echo_config(); 
			_delay_ms(5); //this is required as well. Do not remove this please
			object_distance[0] = measure_distance();
			object_distance[1] = measure_distance();
			object_distance[2] = (unsigned int)((object_distance[0] + object_distance[1])/2);
			//^ multiple measurements then average the result

			audio_feedback(object_distance[2]); //if the object is in range, play a tone
			printf("Object distance = %u. Angle = %d\n ", object_distance[2], motor_angle);
			timer1_servo_config(); //set timer1 to servo mode.
			_delay_ms(15); //this is to prevent motor jerking around
			rotate_motor(delta_theta);
		}
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
	printf("\n"); 
}
void ports_config(){
	//****PORTS FOR SERVO MOTOR****
	DDRD |= (1<<PD4); //PD4 (OC1B) is pwm control signal for servo
	//*****PORTS FOR ECHOLOCATOR******
	DDRB |= (1<<PB1); //PB1 used as trigger signal for sonic ranger
	//******PORTS FOR AUDIO SPEAKER******
	DDRD |= (1<<PD7); //pwm signal output to speaker driver circuit.
	PORTD &= ~(1<<PD7); //prevent audio signal until called.
}

void timer1_servo_config(){
	TCCR1A = (1<<WGM11)|(1<<WGM10)|(1<<COM1B1); 
	TCCR1B = (1<<WGM13)|(1<<CS11); //8prescaler, phase correct pwm: top@OCR1A
	OCR1A = (unsigned int)(fclk_8*servo_period/2);
	//divide by 2 because TCNT1 phase-correct mode counts up then counts down
}

void timer1_echo_config(){
	TCCR1B = (1<<CS11)|(1<<ICES1)|(1<<ICNC1); //8prescale,IC mode,noise cancel
	TCCR1A = 0; //clear regsister from echo mode.
	TIMSK1 = (1<<ICIE1); //enble input capture in mask 
}

void timer2_speaker_config(){
	TCCR2A |= (1<<WGM21); //CTC mode top at OCR2A
	TCCR2A |= (1<<COM2A0); //enable OC2A toggle at CTC event
	TCCR2B |= (1<<CS22)|(1<<CS20); //128 Prescaler 
}

void reset_config(){
	EIMSK |= (1<<INT0);
	EICRA |= (1<<ISC00)|(1<<ISC01); //rising edge triggers interrupt
}
void audio_feedback(unsigned int distance){
	if(distance < max_distance){ //If the object is in range...
		float audio_frequency = 261.6 + 0.581*distance;
		OCR2A = (unsigned short)(fclk_128/(2*audio_frequency));
		DDRD |= (1<<PD7); //enable pwm signal to speaker
		_delay_ms(3000);
		DDRD &= ~(1<<PD7); //disable pwm signal to speaker
	}
}

int measure_distance(){
	PORTB |= (1<<PB1); //write pin B1 high, wait 10microsec, write PB1 low.
	_delay_us(10);
	PORTB &= ~(1<<PB1);
	TCCR1B = (1<<ICES1)|(1<<ICNC1)|(1<<CS11); //set edge detection to rising edge
	return ((int)((ICR1)*distance_factor_8*1.1)); //input capture sets ICR1 value
	//additional factor of 1.1 to account for non-nominal performance
}


void rotate_motor(unsigned short angle){
	OCR1B += (unsigned int)(duty_per_degree*OCR1A*angle);
	motor_angle += angle;
	if(OCR1B > (unsigned int)(OCR1A*max_duty_cyc)){
		OCR1B = (unsigned int)(min_duty_cyc*OCR1A);
		motor_angle = -90; //reset variable to initial position	
	}
}

