#include <avr/io.h>
#include <util/delay.h>
const float fclk = 14.7546e6/1024;
const float fservo = 60.0;
// initialize PWM
void pwm_init()
{
	// initialize timer0 in PWM mode
	TCCR0A |= (1<<WGM00)|(1<<WGM01); //fast pwm mode. TOP = 0xFF
	TCCR0A |=(1<<COM0B1)|(1<<COM0A1); //non-inverted pwm waveform
	TCCR0B |=(1<<CS00)|(1<<CS02); //1024 prescaler
	
	// make sure to physically connect to the OC0A pin (pin PB3)
	DDRB |= (1<<PB3);
}

int main(void)
{	
	uint8_t zero_deg_pulse =(int)(fclk*0.49e-3);
	uint8_t ninety_deg_pulse =(int)(fclk*1.325e-3);
	uint8_t fifteenDegree =(int)(fclk*0.139e-3);
	
	// initialize TIMER0 in fast (non-inverted) PWM mode
	pwm_init();
	while(1)
	{
	
		//start position of the servo motor is at far left (0 deg reference)
		for (int x = zero_deg_pulse; x < ninety_deg_pulse; x += fifteenDegree)
		{
			OCR0A = x; 
			_delay_ms(1000);
			//increment the servo angle by fifteen degrees once per second
		}
		
		for ( int x = ninety_deg_pulse; x > zero_deg_pulse; x -= fifteenDegree)
		{
			OCR0A =x;
			_delay_ms(1000);
			//decrement the servo angle by fifteen degrees once per second
		}
		
	}
}
