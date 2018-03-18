#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include <limits.h>
#include <stdbool.h>

#include "config.h"
#include "bits.h"
#include "twi.h"


bool twi_loader(twi_direction_t direction __attribute__((unused))) {
	return true;
}

bool twi_reader(volatile uint8_t* b) {
	*b = OCR1A;
	return false;
}

bool twi_writer(volatile uint8_t* const b) {
	OCR1A = *b;
	return false;
}


int main() {
	// Configure PWM outputs
	bit_set(DDRB, PB1); // PB3 (OC1A) is output
	bit_clr(PORTB, PB1); // ... and off
	bit_set(DDRB, PB4); // PB4 (OC1B) is output
	bit_clr(PORTB, PB4); // ... and off

	TCCR1 = (0 << CTC1)            // Run in full cycles (from 0 to 255)
	      | (1 << PWM1A)           // Enable Modulator A
	      | (1 << COM1A1)          // OC1x clear on 0, set on match
	      | (1 << COM1A0)          // ... and inverted output disconencted
	      | (PWM_PRESCALER & 0x0F) // Set prescaler form config
	      ;
	
	// Configure and enable I2C
	twi_init(TWI_ADDRESS,
	         twi_loader,
	         twi_reader,
	         twi_writer);

	// Go go, interrupts go!
	sei();

	// Main control loop
	for (;;);
}

