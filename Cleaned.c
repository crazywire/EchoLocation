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

//clock frequencies at various prescalers
#define fclk_1 14745600
#define fclk_8 1843200
#define fclk_64 230400
#define fclk_128 115200
#define fclk_256 57600
#define fclk_1024 14400

#define spd_sound 34300 //units in cm per sec
#define distance_factor_8 spd_sound/(2*fclk_8) 
//distance conversion from clock counts to cm at prescaler of 8

#define servo_period 0.02 //20ms
#define min_duty_cyc 0.028 //experimentally determined value
#define max_duty_cyc 0.12 //experimentally determined value
#define duty_per_degree ((max_duty_cyc-min_duty_cyc)/180)

short motor_angle = -90;
unsigned int object_distance = 0; //units in cm












//FUNCTION PROTOTYPES
int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);
void init_uart(void);
void timer1_echo_config();
void timer1_servo_config();
void timer2_config();
void ports_config();
void audio_feedback(unsigned int x);
void rotate_motor(unsigned short angle);
int measure_distance();

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





//**************************************************************************************************************************************************************
//***********************MAIN***********************************************************************************************************************************
int main(void){
	init_uart();
	ports_config();
	timer1_echo_config(); //echo device
	
	timer2_config(); //audio feedback from speaker
	sei();
	
	timer1_servo_config();
	
	
	OCR1B = (unsigned int)(min_duty_cyc*OCR1A); //min position
	

																										
	while (1){
		timer1_echo_config();
		object_distance = measure_distance();
//		audio_feedback(object_distance); //3000ms audio pulse
//		printf("distance = %u, angle = %d \n", object_distance, motor_angle);
		timer1_servo_config(); //set timer1 to servo mode.
		rotate_motor(15);
		printf("OCR1B = %u\n", OCR1B);
		_delay_ms(300);
		
		
	
	
	}
	
	
																													
	
	
	
}
//**************************END OF MAIN***************************
//***************************************************************




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
	DDRD |= (1<<PD4); 
	//PD4 is used as the pwm control signal on the motor.
	//OC1B = PD4

	//*****PORTS FOR ECHOLOCATOR******
	DDRB |= (1<<PB1); //set PB1 to output 
	//PB1 is used as the trigger signal for the HY-SRF05 sonic ranger

	//******PORTS FOR AUDIO SPEAKER******
	DDRD |= (1<<PD7); 
	PORTD &= ~(1<<PD7); 
	//make sure no signal is outputted from PD7 until we call audio_feedback().
	
	//PD7 is the pwm control for the audio speaker.
	//PD7 = OC2A
	
	
}

void timer1_servo_config(){
	
	TCCR1A = (1<<WGM11)|(1<<WGM10)|(1<<COM1B1); 
	//Phase correct pwm, top at OCR1A
	//Set OC1B on downcount, clear on upcount
	
	
	TCCR1B = (1<<WGM13)|(1<<CS11); 
	//phase correct pwm, top at OCR1A
	//8 prescaler
	
	OCR1A = (unsigned int)(fclk_8*servo_period/2);
	//divide by 2 because TCNT1 in phase correct counts up then counts down
}


void timer1_echo_config(){
	//TIMER1 IS USED FOR THE ECHOLOCATOR

	//64 prescaler
	//set input capture detection to rising edge
	//enable input capture interrupt in TIMSK1
	
	TCCR1B = (1<<CS11)|(1<<ICES1);
	TCCR1B |= (1<<ICES1); //rising edge select 
	TIMSK1 |= (1<<ICIE1); //enble input capture in mask 
	
}

void timer2_config(){
	//TIMER2 IS USED FOR THE AUDIO SPEAKER
	
	TCCR2A |= (1<<WGM21); //CTC mode top at OCR2A
	TCCR2A |= (1<<COM2A0); //enable OC2A toggle at CTC event
	
	
	TCCR2B |= (1<<CS22)|(1<<CS20); //128 Prescaler 
	//so that OCR0A goes from fclk_128/(2*audio_freq min) 
	//to fclk_128/(2*audio_freq max).
	//min audio frequency is 291Hz, max is 493Hz
	//This puts OCR2A ~ [198 ->>117]
}
void audio_feedback(unsigned int distance){
	//this function is meant to take in the object distance and play a tone of varying pitch 
	//...based on that frequency
	
	float audio_frequency = 261.6; //default initialized value
	
	 // {261.6, 282.7, 303.8, 324.9, 346, 367.3, 388.3, 409.4, 430.5, 451.7, 472.8, 493.9};
	if((distance >= 0)&&(distance <= 36)){audio_frequency = 261.6;}
	if((distance > 36)&&(distance <= 73)){audio_frequency = 282.7;}
	if((distance > 73)&&(distance <=109)){audio_frequency = 303.8;}
	if((distance > 109)&&(distance <= 145)){audio_frequency = 324.9;}	
	if((distance > 145)&&(distance <= 182)){audio_frequency = 346;}
	if((distance > 182)&&(distance <= 218)){audio_frequency = 367.3;}
	if((distance > 218)&&(distance <= 254)){audio_frequency = 388.3;}
	if((distance >254)&&(distance <= 291)){audio_frequency = 409.4;}
	if((distance >291)&&(distance <= 327)){audio_frequency = 430.5;}
	if((distance >327)&&(distance <= 364)){audio_frequency = 451.7;}
	if((distance >364)&&(distance <= 400)){audio_frequency = 472.8;}
	if(distance > 400){audio_frequency = 493.9;}
		
	OCR2A = (unsigned short)(fclk_128/(2*audio_frequency));
	DDRD |= (1<<PD7); //enable pwm signal to speaker
	 _delay_ms(3000);
	DDRD &= ~(1<<PD7); //disable pwm signal to speaker
}

int measure_distance(){
	
	//****THIS PART TRIGGERS THE SIGNAL TO THE SONIC RANGER****
	PORTB |= (1<<PB1); //write pin B1 high, wait 10microsec, write PB1 low.
	_delay_us(10);
	PORTB &= ~(1<<PB1);
	TCCR1B = (1<<ICES1)|(1<<ICNC1)|(1<<CS11); //set edge detection to rising edge
	//*** INPUT CAPTURE ISR HAPPENS OUT OF FUNCTION, THEN WE RETURN HERE *****
	
	return ((int)((ICR1)*distance_factor_8*1.1)); 
	//additional factor of 1.1 to account for non-nominal performance
}


void rotate_motor(unsigned short angle){
	
	if(OCR1B < (unsigned int)(max_duty_cyc*OCR1A)){
		
		OCR1B +=(unsigned int)(duty_per_degree*angle*OCR1A);
	}
	
	
	
	
	
	
	
	
}
