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
const float max_pos_t = 2.35e-3; //4.8ms

unsigned short motor_angle = 0;
unsigned short echo_flag = 0;
unsigned int pulse_counts = 0;
unsigned int object_distance = 0; //units in cm


volatile unsigned short reset_flag = 0;







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
	
	reset_flag = 1;
	printf("external interrupt\n");

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
	DDRB |= (1<<PB4); //set PB4 to output.
	//PB4 is used as the pwm control signal on the motor.
	//OC0B = PB4

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
	
	TCCR2A |= (1<<WGM21); //CTC mode top at OCR2A
	TCCR2A |= (1<<COM2A0); //enable OC2A toggle at CTC event
	
	
	TCCR2B |= (1<<CS22)|(1<<CS20); //128 Prescaler 
	//so that OCR0A goes from fclk_128/(2*audio_freq min) 
	//to fclk_128/(2*audio_freq max).
	//min audio frequency is 291Hz, max is 493Hz
	//This puts OCR2A ~ [198 ->>117]
	
	//OCR2A = (int)(fclk_128/(2*audio_frequency[1]));
	
	

}

void reset_config(){
	
	EIMSK |= (1<<INT0);
	EICRA |= (1<<ISC01)|(1<<ISC00);
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
	
	const float min_duty_cyc = min_pos_t/servo_period;
	const float max_duty_cyc = max_pos_t/servo_period;
	const float duty_per_deg = (max_duty_cyc - min_duty_cyc / 180);
	//divide by 180 because 180 degrees of rotation for the motor.
	
	unsigned short delta_theta = 15; 
	//this variable controls the angle increment when the motor rotates	
	
    while (1){
		if(reset_flag){
			
			reset_flag = 0;
			//clear variables at the start of each loop cycle.
			motor_angle = 0;
			pulse_counts = 0;
			object_distance = 0;
		
		
			for(unsigned int x = (int)(min_duty_cyc*OCR0A); x <= (int)(max_duty_cyc*OCR0A); x += (int)(duty_per_deg*delta_theta)){
				OCR0B = x;
				_delay_ms(350);
				trigger_echo();
				pulse_counts = Falling_Edge_cnt -Rising_Edge_cnt ;
				object_distance = (int)((pulse_counts*4.4)/58); 
				//conversion factor of 4/58 converted to 4.4/58 in lab to account for 
				//....real life variance of HY-SRF05 performance.
	
				if(object_distance > 410){
					//410 cm = 400 cm +- 2.5% error
					DDRD &= ~(1<<PD7); //disable speaker output
				
					printf("Object out of range. Angle = %u.\n", motor_angle); 				
				
				
				}
				else{
					printf("Object distance = %u cm. Angle = %u.\n", object_distance, motor_angle);
					OCR2A = audio_feedback(object_distance); 
					//set OCR2A to the appropriate value for the distance of the object
					//so the audio frequency from the speaker corresponds to the object distance
				
				
					DDRD |= (1<<PD7); //enable pwm signal to speaker
					//_delay_ms(150); //brief pause
					//DDRD &= ~(1<<PD7); //disable pwm signal to speaker
					//_delay_ms(350);
			
				}
			
		
				motor_angle += delta_theta;
			}
			
			OCR0B = (int)(min_duty_cyc*OCR0A); //reset back to min position
		}
		
		
		
		
		
		
		
		
    }
}
