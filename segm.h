#include <stdint.h>
#include <stdbool.h>

/** 
 * Table, mapping numbers to bits for each segment of dispay
 * For full ASCII table see https://github.com/dmadison/LED-Segment-ASCII
 */
extern const uint8_t segm_sym_table[];

/**
 * Stores special symbols to save space in symbol table
 */
enum {
	segm_sym_DASH = 0x02,	/* Hyphen	 */
	segm_sym_DP = 0x01	/* Decimal point */
};

/**
 * segm_bcd() - convert binary number to BCD representation
 * @number:	value to be converted
 * @res:	byte array
 *
 * Non-packed BCD. User must ensure array has enough size.
 */
void segm_bcd(uint16_t number, uint8_t *res);

/**
 * GPIO abstraction layer
 * For more: http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_port_pass
 */
struct segm_Port {
	volatile uint8_t *DDR;	/* addr of GPIO DDRx direction register */
	volatile uint8_t *PIN;	/* addr of GPIO PINx input register */
	volatile uint8_t *PORT;	/* addr of GPIO PORTx data register */
};

struct segm_Pin {
	struct segm_Port *port;	/* GPIO port */
	uint8_t pin;		/* number of pin in GPIO port */
};

struct segm_Display {
	struct segm_Pin SHCP;	/* 74HC595: SHCP pin location */
	struct segm_Pin STCP;	/* 74HC595: STCP pin location */
	struct segm_Pin DS;	/* 74HC595: DS pin location */
	void (*delay_func)(uint8_t);	/* pointer to delay function */
	void (*sleep_ms_func)(uint16_t);	/* millisecond sleep func ptr */
	bool is_comm_anode;	/* Inverts on comm cathode */
};


/**
 * segm_init() - initialize GPIO for 74HC595 shift register
 * @display:	GPIO configuration structure
 */
void segm_init(struct segm_Display *display);


/**
 * segm_shiftbyte() - shift one byte to 74HC595
 * @display:	GPIO configuration structure
 * @byte:	a byte to be shifted
 */
void segm_shiftbyte(struct segm_Display *display, uint8_t byte);


/**
 * segm_latch() - latch output on 74HC595
 * @display:	GPIO configuration structure
 */
void segm_latch(struct segm_Display *display);


/**
 * segm_latch() - perform dynamic indication of 4 symbols on display
 * @display:	GPIO configuration structure
 * @arr4:	array of 4 symbols (7SEGM: [4][3][2][1])
 * Inversion for common cathode displays is performed automatically
 */
void segm_indicate4(struct segm_Display *display, uint8_t *arr4);