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
const float fclk_1 = 14745600;
const float fclk_8 = 1843200;
const float fclk_64 = 230400;
const float fclk_128 = 115200;
const float fclk_256 = 57600;
const float fclk_1024 = 14400;

const float servo_period = 35e-3; //35ms
//35 ms is longest time period that will fit in 2*256 cnts at 1024 prescaler.

//PWM pulse lengths (in seconds) for min and max servo motor positions.
const float min_pos_t = 0.6e-3; //1ms
const float max_pos_t = 2.5e-3; //4.8ms

short motor_angle = 0;
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


ISR(INT0_vect){
	
	

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
	
	
	//******************PORTS TO CALL THE SERVO MOTOR ATMEGA BOARD******
	
	DDRD |= (1<<PD5); //PD5 = OC1A, PD4 = OC1B
	
	DDRB |= (1<<PB7); //set PB7 to output.
	PORTB &= ~(1<<PB7); //write PB7 low
	
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

void timer0_config(){
	
	
	 //fast PWM, top at OCR0A
	TCCR0A |= (1<<WGM01)|(1<<WGM00);
	TCCR0B |= (1<<WGM02);
	
	
	
	OCR0A = 100; //TOP
	
	TCCR0B |= (1<<CS00)|(1<<CS02); //1024 prescaling
	
	TCCR0A |= (1<<COM0A1);
	
	
	
	
	
	
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
	
	TCCR2A |= (1<<WGM21); //CTC mode top at OCR2A
	TCCR2A |= (1<<COM2A0); //enable OC2A toggle at CTC event
	
	
	TCCR2B |= (1<<CS22)|(1<<CS20); //128 Prescaler 
	//so that OCR0A goes from fclk_128/(2*audio_freq min) 
	//to fclk_128/(2*audio_freq max).
	//min audio frequency is 291Hz, max is 493Hz
	//This puts OCR2A ~ [198 ->>117]
	
	
	
	

}

void reset_config(){
	
	EIMSK |= (1<<INT0); //enable INT0 in the interrupt mask
	EICRA |= (1<<ISC01)|(1<<ISC00); //rising edge of INT0 triggers the ISR
}
unsigned short audio_feedback(unsigned int distance){
	//this function is meant to take in the object distance and return the....
	//.....required value for OCR2A that will generate the correct frequency
	unsigned short ctc_top_count = 0; 
	float audio_frequency = 261.6;
	
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
	
	
	ctc_top_count = (unsigned short)(fclk_128/(2*audio_frequency));
	return ctc_top_count;
}



void servo_call(){
	if((PORTB & (1<<PB7)) == (1<<PB7)){
		
		PORTB &= ~(1<<PB7); //write PB7 low
	}
	
	if((PORTB & (1<<PB7)) == 0x00){
		
		
		PORTB |= (1<<PB7); //write PB7 high
	}
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
	reset_config();
	
	timer0_config();
	timer1_config();
	timer2_config();
	sei();
	
	
	
	
	
    while (1){
	
		//clear variables at the start of each loop cycle.
		pulse_counts = 0;
		object_distance = 0;

		trigger_echo();
		pulse_counts = Falling_Edge_cnt -Rising_Edge_cnt ;
		object_distance = (int)((pulse_counts*4.4)/58); 
		//conversion factor of 4/58 converted to 4.4/58 in lab to account for 
		//....real life variance of HY-SRF05 performance.
	
		if(object_distance > 405){
			//400 cm with some error
			DDRD &= ~(1<<PD7); //disable speaker output
			printf("Object out of range.\n");								
		}
		else{
			printf("Object distance = %u cm. \n", object_distance);
			OCR2A = audio_feedback(object_distance); 
			//set OCR2A to the appropriate value for the distance of the object
			//so the audio frequency from the speaker corresponds to the object distance
				
				
			DDRD |= (1<<PD7); //enable pwm signal to speaker
			_delay_ms(150); //brief pause
			DDRD &= ~(1<<PD7); //disable pwm signal to speaker
			_delay_ms(350);
			}
		servo_call();
		_delay_ms(350);
    }
}
