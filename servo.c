/*
* uart_test.c
*
* Created: 2017-01-03 10:27:53 AM
* Author : JF
*/
#include <stdio.h>
#include <avr/io.h>
#include <util/delay

const float fclck = 14.7546e6;
const float fservo = 50.0e6;
const float delta_t = 1.0e-3;
const float zeroDegrees = 1.0e-3;
const float nintyDegrees = 1.5e-3;
const float OneEightydegrees = 2.0e-3;
//Simple Wait Function
void delay(int delay)
{
   int i=0;
   for(i=0;i<delay;i++){
	   TCCR0A=0;
	   TCCR0B = (1<<CS00) | (1<<CS02);
	   TCNT0=0;
	   OCR0A = (int)(fclck*delt_t -1);
	   TIFR1 = (1<<OCF1A);
	   while(!(TIFR1 &(1<<OCF1A);
   }
   TCCR0B =0;
}

void main()
{
   //Configure TIMER1
   TCCR1A|=(1<<COM1A1)|(1<<COM1B1)|(1<<WGM11);        //NON Inverted PWM
   TCCR1B|=(1<<WGM13)|(1<<WGM12)|(1<<CS11)|(1<<CS10); //PRESCALER=64 MODE 14(FAST PWM)

   ICR1A= (int)(fclck/fservo -1);  //fPWM=50Hz (Period = 20ms Standard).

   DDRD|=(1<<PD4)|(1<<PD5);   //PWM Pins as Out
   //if you want use PIN D4, use 0CR1B in the while loop. This way you can run two servos.

   while(1)
   {

      OCR1A=; zeroDegrees*fclck;  //0 degree
      delay(500);
      OCR1A=nintyDegrees*fclck;  //90 degree
      delay(500);
      OCR1A=OneEightydegrees*fclck;  //180 degree
	  delay(500);
   }
}
