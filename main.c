#include <stdio.h>
#include <atmel_start.h>
#include <pwm_basic.h>
#include <util/delay.h>
#include <atomic.h>
#include <avr/pgmspace.h>


//volatile uint16_t         PWM_0_isr_executed_counter = 0;
volatile PWM_0_register_t PWM_0_duty = 14;
volatile uint16_t pulse_width =0;
int distance =0, angle = 0;

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

int main(void)
{
	//initialize the system
	atmel_start_init();
	// Enable pin output
	PWM_0_enable_output_ch0();
	// Set channel 0 duty cycle value register value to specified value
	PWM_0_load_duty_cycle_ch0(PWM_0_duty);
	// Set counter register value
	PWM_0_load_counter(0);
	
	ENABLE_INTERRUPTS();
	
	while(1){
		// Wait for ISR to be executed 65000 times
		for(PWM_0_duty = 14; PWM_0_duty < 29; PWM_0_duty++){
			PWM_0_load_duty_cycle_ch0(PWM_0_duty);
			_delay_ms(500);
			
			angle += angle;
			
			pulse_width = t2-t1;
			
			//clock resolution = 1/(F_CPU*64) = 4.33uS
			//distance = pulse_with*fresolution/58
			distance = (int)pulse_width/58;
			printf("Distance = %d cm \n", distance);
			printf("Angle = %d deg", angle);
			distance =0;
		}
		angle=0;
	}
}