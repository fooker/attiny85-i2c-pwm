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
	return true;
}

bool twi_writer(volatile uint8_t* const b) {
	OCR1A = *b;
	return true;
}


int main() {
	// Configure PWM outputs
	bit_set(DDRB, PB1); // PB3 (OC1A) is output
	bit_clr(PORTB, PB1); // ... and off

	TCCR1 = (0 << CTC1)            // Run in full cycles (from 0 to 255)
	      | (1 << PWM1A)           // Enable Modulator A
	      | (1 << COM1A1)          // OC1x set on 0, clear on match
	      | (0 << COM1A0)          // ... and inverted output disconencted
	      | (PWM_PRESCALER & 0x0F) // Set prescaler form config
	      ;
	OCR1A = 0x00;
	
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

