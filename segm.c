#include "segm.h"
#include <stdint.h>
#include <stdbool.h>

static const uint8_t stddelay = 2;
static const uint16_t sym_delay = 1;	/* 500 / Hz */

const uint8_t segm_sym_table[] = {
	/* Common cathode here */
	[0] = 0x3F,
	[1] = 0x06,
	[2] = 0x5B,
	[3] = 0x4F,
	[4] = 0x66,
	[5] = 0x6D,
	[6] = 0x7D,
	[7] = 0x07,
	[8] = 0x7F,
	[9] = 0x6F
};


void segm_bcd(uint16_t number, uint8_t *res)
{
	for (uint8_t i = 0; i < 4; i++) {
		res[3-i] = number % 10;
		number = number / 10;
	}
}


void BIN2BCD(uint8_t *buffer, uint8_t n)
{
	buffer[0]=0;
	while(n>=10) {buffer[0]++;n-=10;}
}


void segm_init(struct segm_Display *display)
{
	struct segm_Pin *ptrarr[] = {
		&display->SHCP,
		&display->STCP,
		&display->DS,
	};

	for (int i = 0; i < (uint8_t)(sizeof ptrarr / sizeof *ptrarr); i++) {
		*(ptrarr[i]->port->PORT) &= ~(1 << ptrarr[i]->pin);
		*(ptrarr[i]->port->DDR) |= 1 << ptrarr[i]->pin;
	} 
}


void segm_shiftbyte(struct segm_Display *display, uint8_t byte)
{
	bool bit;
	for (int i = 0; i < 8; i++) {
		bit = byte >> 7;
		byte = byte << 1;
		/* Set DS pin to bit	*/
		if (bit)
			*(display->DS.port->PORT) |= 1 << display->DS.pin;
		else
			*(display->DS.port->PORT) &= ~(1 << display->DS.pin);
		/* Drive low-to-high posedge on SHCP pin */
		*(display->SHCP.port->PORT) &= ~(1 << display->SHCP.pin);
		(*display->delay_func)(stddelay);  /* Call delay with 1 step */
		*(display->SHCP.port->PORT) |= 1 << display->SHCP.pin;
		(*display->delay_func)(stddelay);
	}
}


void segm_latch(struct segm_Display *display)
{
	/* Drive low-to-high posedge on STCP pin */
	*(display->STCP.port->PORT) &= ~(1 << display->STCP.pin);
	(*display->delay_func)(stddelay);  /* Call delay with 1 step */
	*(display->STCP.port->PORT) |= 1 << display->STCP.pin;
	(*display->delay_func)(stddelay);

}


void segm_indicate4(struct segm_Display *display, uint8_t *arr4)
{
	uint8_t sym;
	uint8_t digit = 0x01;  /* [_ _ _ _ 4 3 2 1] -> 7SEGM:[4] [3] [2] [1] */
	for (int i = 3; i >= 0; i--) {
		sym = arr4[i];
		if (!display->is_comm_anode)
			sym = ~sym;
		segm_shiftbyte(display, sym);
		segm_shiftbyte(display, digit);
		digit = digit << 1;
		segm_latch(display);
		(*display->sleep_ms_func)(sym_delay);
	}
}



