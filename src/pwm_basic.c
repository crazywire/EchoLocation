/**
 * \file
 *
 * \brief PWM Basic driver implementation.
 *
 *
 * Copyright (C) 2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 *
 */

#include <pwm_basic.h>

pwm_irq_cb_t PWM_0_cb = NULL;

/**
 * \brief Initialize PWM
 */
int8_t PWM_0_init()
{

	/* Enable TC0 */
	PRR0 &= ~(1 << PRTIM0);

	TCCR0A = (1 << COM0A1) | (0 << COM0A0)   /* Clear OCA on Compare Match, set OCA on BOTTOM (non-inverting mode) */
	         | (1 << COM0B1) | (0 << COM0B0) /* Clear OCB on Compare Match, set OCB on BOTTOM (non-inverting mode) */
	         | (1 << WGM01) | (1 << WGM00);  /* TC8 Mode 3 Fast PWM */

	TCCR0B = 0                                          /* TC8 Mode 3 Fast PWM */
	         | (1 << CS02) | (0 << CS01) | (1 << CS00); /* IO clock divided by 1024 */

	TIMSK0 = 0 << OCIE0B   /* Output Compare B Match Interrupt Enable: disabled */
	         | 0 << OCIE0A /* Output Compare A Match Interrupt Enable: disabled */
	         | 1 << TOIE0; /* Overflow Interrupt Enable: enabled */

	// OCR0A = 0x0; /* Output compare A: 0x0 */

	// OCR0B = 0x0; /* Output compare B: 0x0 */

	return 0;
}

void PWM_0_enable()
{
}

void PWM_0_disable()
{
}

void PWM_0_enable_output_ch0()
{

	TCCR0A |= ((1 << COM0A1) | (0 << COM0A0));
}

void PWM_0_disable_output_ch0()
{

	TCCR0A &= ~((0 << COM0A1) | (0 << COM0A0));
}

void PWM_0_enable_output_ch1()
{

	TCCR0A |= ((1 << COM0B1) | (0 << COM0B0));
}

void PWM_0_disable_output_ch1()
{

	TCCR0A &= ~((0 << COM0B1) | (0 << COM0B0));
}

void PWM_0_load_counter(PWM_0_register_t counter_value)
{
	TCNT0 = counter_value;
}

void PWM_0_load_duty_cycle_ch0(PWM_0_register_t duty_value)
{
	OCR0A = duty_value;
}

void PWM_0_load_duty_cycle_ch1(PWM_0_register_t duty_value)
{
	OCR0B = duty_value;
}

void PWM_0_register_callback(pwm_irq_cb_t f)
{
	PWM_0_cb = f;
}

ISR(TIMER0_OVF_vect)
{
	static volatile uint8_t callback_count = 0;

	// Clear the interrupt flag
	TIFR0 |= (1 << TOV0);

	// callback function - called every 0 passes
	if ((++callback_count >= PWM_0_INTERRUPT_CB_RATE) && (PWM_0_INTERRUPT_CB_RATE != 0)) {
		if (PWM_0_cb != NULL) {
			PWM_0_cb();
		}
	}
}
