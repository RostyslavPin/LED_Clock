#ifndef F_CPU
#define F_CPU   16000000UL
#endif

#include "segm.h"
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>

static void soft_delay(volatile uint8_t N)
{
	/* If volatile is not used, AVR-GCC will optimize this stuff out     */
	/* making our function completely empty                              */
	volatile uint8_t inner = 0x01;
	while (N--) {
		while (inner--);
	}
}

/** Timer2 Interrupt (on overflow), see datasheet
 * For vectors, refer to <avr/iom328p.h>
 * For more on interrupts handling with AVR-GCC see
 * https://www.nongnu.org/avr-libc/user-manual/group__avr__interrupts.html
 */
ISR(TIMER2_OVF_vect, ISR_BLOCK)
{
	TCCR2B &= ~((1 << CS22) | (1 << CS21) | (1 << CS20)); /* stop timer */
	/* It's often required to manually reset interrupt flag */
	/* to avoid infinite processing of it.                  */
	/* not on AVRs (unless OCB bit set)                     */
	/* 	TIFR2 &= ~TOV2;                                 */
}


void sleep_ms(uint16_t ms_val)
{
	/* Set Power-Save sleep mode */
	/* https://www.nongnu.org/avr-libc/user-manual/group__avr__sleep.html */
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);
	cli();		/* Disable interrupts -- as memory barrier */
	sleep_enable();	/* Set SE (sleep enable bit) */
	sei();  	/* Enable interrupts. We want to wake up, don't we? */
	TIMSK2 |= (1 << TOIE2); /* Enable Timer2 Overflow interrupt by mask */
	while (ms_val--) {
		/* Count 1 ms from TCNT2 to 0xFF (up direction) */
		TCNT2 = (uint8_t)(0xFF - (F_CPU / 128) / 1000);

		/* Enable Timer2 */
		TCCR2B =  (1 << CS22) | (1 << CS20); /* f = Fclk_io / 128, start timer */

		sleep_cpu();	/* Put MCU to sleep */

		/* This is executed after wakeup */

	}
	sleep_disable();	/* Disable sleeps for safety */
}


static struct segm_Port PB = {
	.DDR = &DDRB,
	.PIN = &PINB,
	.PORT = &PORTB,
};

static struct segm_Display display = {
	.SHCP = {.port = &PB, .pin = 0},
	.STCP = {.port = &PB, .pin = 1},
	.DS   = {.port = &PB, .pin = 2},
	.delay_func = &_delay_loop_1,	/* 3 cycles / loop, busy wait */
	.sleep_ms_func = &sleep_ms,	/* 3 cycles / loop, busy wait */
	.is_comm_anode = false		/* We have common cathode display */
};


bool poll_btn_low(struct segm_Pin *btn)
{
	if (!(*(btn->port->PIN) & (1 << btn->pin))) {
		sleep_ms(10);
		if (!(*(btn->port->PIN) & (1 << btn->pin)))
			return true;
	}
	return false;
}

struct segm_Pin btn1 = {.port = &PB, .pin = 3},
btn2 = {.port = &PB, .pin = 4};

uint8_t second = 0;
uint8_t time_cntr = 0; // Global variable for number of Timer 0

/** TIMER0 overflow interrupt service routine
* called whenever TCNT0 overflows
*/
ISR(TIMER0_OVF_vect) { 
	TCNT0 = 239; // Initialize counter value to 239
	time_cntr++;
	if (time_cntr == 250) // Check for one second, increment counter
	{
		time_cntr = 0;
		second++;
	}
}

int main(void)
{
	segm_init(&display);
	uint8_t hour = 11;
	uint8_t minute = 59;

	TCCR0A = 0; // Set entire TCCR0A register to 0
	TCCR0B |= (1 << CS02); // Set prescaler to 256 and start the timer
	TIMSK0 |= (1 << TOIE0); // Enable overflow interrupt for Timer0
	sei(); // Set global interrupt enable

	/* Configure buttons for input with pullup R */
	*(btn1.port->DDR) &= ~(1 << btn1.pin);
	*(btn1.port->PORT) |= (1 << btn1.pin);
	*(btn2.port->DDR) &= ~(1 << btn2.pin);
	*(btn2.port->PORT) |= (1 << btn2.pin);

	uint8_t test[] = {0x3F, 0x3F, 0x3F, 0x3F};

	uint8_t *symbols = test;
	while (1) {
		test[0] = segm_sym_table[hour / 10];
		test[1] = segm_sym_table[hour % 10] | 0x80;
		test[2] = segm_sym_table[minute / 10];
		test[3] = segm_sym_table[minute % 10];
		if (second > 59)
		{
			second = 0;
			minute++;
			if (minute > 59)
			{
				minute = 0;
				hour++;
				if (hour > 23)
				{
					hour = 0;
				}
			}
		}
		if (poll_btn_low(&btn1)) {
			soft_delay(250);
			hour++;
			soft_delay(250);
			if (hour >= 24)
				hour = 0;
		} else if (poll_btn_low(&btn2)) {
			soft_delay(250);
			minute++;
			soft_delay(250);
			if (minute >= 60)
				minute = 0;
		}
		segm_indicate4(&display, symbols);
	}
}