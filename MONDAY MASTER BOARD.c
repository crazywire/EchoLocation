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

volatile short ext_flag = 0;

volatile uint16_t Rising_E_time=0, Falling_E_time=0;
volatile uint16_t time_elapsed=0; //The time between the falling and rising edge
unsigned int object_distance = 0;

//FUNCTION PROTOTYPES
int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);
int measure_distance();
void init_uart(void);
void port_config();
void timer1_config();
void interrupt_cfg();
void timer2_config(); 
void audio_feedback();
void servo_call();

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
	ext_flag = 1;
}




int main(void){

	init_uart();
	port_config();
	timer1_config();
	timer2_config();
	interrupt_cfg();
	sei();
	
	while(1){
		if(ext_flag){
			object_distance = measure_distance();
			//audio_feedback(object_distance);
			_delay_ms(50);
			//printf("Distance = %u,Motor angle = %d.\n", object_distance , motor_angle);
			printf("%d,%u.", motor_angle, object_distance);
			servo_call();
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
	UBRR0 = 95; // configures a baud rate of 115200
	stdout = &mystdout;
	stdin = &mystdin;
	printf("\n");
}

void port_config(){
	//****PORTS FOR SERVO MOTOR****
	DDRB |= (1<<PB7);
	//PB7 is used to call the servo motor.

	//*****PORTS FOR ECHOLOCATOR******
	DDRB |= (1<<PB1); //set PB1 to output 
	//PB1 is used as the trigger signal for the HY-SRF05 sonic ranger

	//******PORTS FOR AUDIO SPEAKER******
	DDRD |= (1<<PD7); //OC2B (For speaker)
	PORTD &= ~(1<<PD7); 
	//make sure no signal is outputted from PD7 until we call audio_feedback().
	
	//PD7 is the pwm control for the audio speaker.
	//PD7 = OC2A
}

void interrupt_cfg(){
	EIMSK |= (1<<INT0);
	EICRA |= (1<<ISC01)|(1<<ISC00); // trigger on only rising edge
	TIMSK1 = (1<<ICIE1);
}

void timer1_config(){
	TCCR1B = (1<<CS11)|(1<<ICES1)|(1<<ICNC1); //8prescaler, phase correct pwm: top@OCR1A
	TCCR1A = 0;
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
	//*** INPUT CAPTURE ISR HAPPENS OUT OF FUNCTION, THEN WE RETURN HERE *****
	_delay_ms(25);
	return ((unsigned int)((ICR1)*distance_factor_8*1.1)); 
	//additional factor of 1.1 to account for non-nominal performance
}







void servo_call(){
		
	PORTB |= (1<<PB7);
	_delay_ms(10);
	PORTB &= ~(1<<PB7);

	motor_angle += 5;
	if(motor_angle > 90){
		motor_angle = -90;
		ext_flag = 0;
	}
}
