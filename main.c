#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include <limits.h>
#include <stdbool.h>

#include "config.h"
#include "bits.h"
#include "twi.h"


volatile uint8_t i2c_setup_flag;

volatile uint8_t M_EN;
volatile uint8_t M_DIR;
volatile uint8_t M_STR;
volatile uint8_t M_PRES;
volatile uint8_t M_OSCR;

volatile uint16_t M_TURNS;


// ISR FUNCTIONS TO COUNT STEPS (TODO)
inline void enable_isr();
inline void disable_isr();



inline void setup_timer(uint8_t presc, uint8_t oscr) {

	TCNT1 = 0;

	TCCR1 = (1 << CTC1)            // Clear on OCRA1
	      | (0 << PWM1A)           // Disable Modulator A
	      | (0 << COM1A1)          // Pin
	      | (1 << COM1A0)          // ... and inverted output disconencted
	      | (presc & 0x0F) 				 // Set prescaler
				;

	OCR1A = oscr;
	OCR1C = oscr;
}

inline void stop_timer() {
	TCNT1 = 0;
	TCCR1 &= ~(0x0F);	// Prescaler 0
}

bool twi_loader(twi_direction_t direction __attribute__((unused))) {
	return true;
}

bool twi_reader(volatile uint8_t* b) {
	*b = (M_EN << 0)
		 | (M_DIR << 1)
		 ;

	return true;
}

bool twi_writer(volatile uint8_t* const b) {
	uint8_t	func = *b >> 4;			// |*F3*|*F4*|*F1*|*F0*| D3 | D2 | D1 | D0 |
	uint8_t data = *b & 0x0F;

	switch(func) {
		case FUNC_SHAFT:
			M_EN = bit_get(data, REG_EN);
			M_DIR = bit_get(data, REG_DIR);
			M_STR	= bit_get(data, REG_START);
			i2c_setup_flag |= 0x01;
			break;

		case FUNC_PRESC:
			M_PRES = data;
			break;

		case FUNC_OCRAL:
			M_OSCR = data;								// LOW NIBBLE OSCR
			break;

		case FUNC_OCRAH:
			M_OSCR |= (data << 4) & 0xF0;				// HIGH NIBBLE OSCR
			i2c_setup_flag |= 0x02;
			break;
	}

	return true;
}


int main() {

	// Configure control outputs
	bit_set(DDRB, DIR_PIN); // is output
	bit_clr(PORTB, DIR_PIN); // ... and off
	bit_set(DDRB, ENA_PIN); // is output
	bit_set(PORTB, ENA_PIN); // ... and on

	// Configure PWM outputs
	bit_set(DDRB, PUL_PIN); // PB3 (OC1A) is output
	bit_clr(PORTB, PUL_PIN); // ... and off

	// Configure and enable I2C
	twi_init(TWI_ADDRESS,
	         twi_loader,
	         twi_reader,
	         twi_writer);

	// ---------------------------- TESTS!!
	//bit_clr(PORTB, EN_PIN);
	//setup_timer(4, 250);
	// enable_isr();
	// ----------------------------

	// Go go, interrupts go!
	i2c_setup_flag = 0;
	sei();



	// Main control loop
	while (true) {
		if (i2c_setup_flag > 0) {
			if (i2c_setup_flag & 0x01) {
				M_EN  ? bit_set(PORTB, EN_PIN)  : bit_clr(PORTB, EN_PIN);
				M_DIR ? bit_set(PORTB, DIR_PIN) : bit_clr(PORTB, DIR_PIN);
			}
			if (i2c_setup_flag & 0x02) {
				setup_timer(M_PRES, M_OSCR);
			}
			i2c_setup_flag = 0;
		}
	}
}





// ISR FUNCTIONS TO COUNT STEPS (TODO)
volatile uint8_t isr_status;

inline void enable_isr() {
        isr_status = 0;
        TIMSK |= (1 << TOIE1);
        TIMSK |= (1 << OCIE1A);
}

inline void disable_isr() {
        TIMSK &= ~(1 << TOIE1);
}

//TIMER 1 COMPARE match A
ISR(TIMER1_COMPA_vect){
	// ------------------- TEST
	if (isr_status) {
		isr_status = 0;
		bit_clr(PORTB, DIR_PIN);
	} else {
		isr_status = 1;
		bit_set(PORTB, DIR_PIN);
	}
	// -------------------
}
