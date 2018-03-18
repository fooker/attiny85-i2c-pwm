#include <avr/io.h>
#include <avr/interrupt.h>


#include "twi.h"
#include "bits.h"


// The device address
static twi_address_t twi_address;
static twi_loader_t twi_loader;
static twi_reader_t twi_reader;
static twi_writer_t twi_writer;


// The current state handler
static enum {
	STATE_ADDR,
	STATE_READ_DATA,
	STATE_READ_ACK,
	STATE_WRITE_DATA,
	STATE_WRITE_ACK
} twi_state;

uint8_t twi_buffer;


void twi_init(twi_address_t address,
			  twi_loader_t loader,
			  twi_reader_t reader,
			  twi_writer_t writer) {
	// Remember the address and the handlers
	twi_address = address;
	twi_loader = loader;
	twi_reader = reader;
	twi_writer = writer;

	// Init USI in 2-wire mode
	USICR = (1 << USISIE) // Disable start condition interrupt
	      | (0 << USIOIE) // Disable overflow interrupt
	      | (1 << USIWM1) // Two-wire mode
	      | (0 << USIWM0) // ... hold SCL low on start condition
	      | (1 << USICS1) // External clock
	      | (0 << USICS0) // ... on positive edge
	      | (0 << USICLK) // ...
	      | (0 << USITC)  // Don't toggle CLK pin
	      ;

	// Clear the USI status
	USISR = (1 << USISIF) // Clear start condition flag
	      | (1 << USIOIF) // Clear overflow condition flag
	      | (1 << USIPF)  // Clear stop condition flag
	      | (1 << USIDC)  // Clear arbitration error flag
	      | 0x00          // Wait for 8 clock cycles
	      ;

	// Configure port
	bit_set(PORTB, PB0); // SDA as tristate
	bit_clr(DDRB, DDB0); // ... with pull-up
	bit_set(PORTB, PB2); // SCL as tristate
	bit_set(DDRB, DDB2); // ... with pull-up
}


ISR(USI_START_vect) {
	// Set SDA as input
	bit_clr(DDRB, DDB0);

	// Ensure start condition has completed
	while (bit_get(PINB, PB2) &&
	       !bit_get(PINB, PB0)) {
	}

	// Check if this start condition was directly followed by a stop condition
	if (!bit_get(PINB, PB0)) {
		// Got a real start condition - reconfigure USI for receiving data
		USICR = (1 << USISIE) // Enable start condition interrupt
		      | (1 << USIOIE) // Enable overflow interrupt
		      | (1 << USIWM1) // Two-wire mode
		      | (1 << USIWM0) // ... hold SCL low on start condition and overflow
		      | (1 << USICS1) // External clock
		      | (0 << USICS0) // ... on positive edge
		      | (0 << USICLK) // ...
		      | (0 << USITC)  // Don't toggle CLK pin
		      ;	

	} else {
		// Got a stop condition - reset
		USICR = (1 << USISIE) // Enable start condition interrupt
		      | (0 << USIOIE) // Disable overflow interrupt
		      | (1 << USIWM1) // Two-wire mode
		      | (0 << USIWM0) // ... hold SCL low on start condition
		      | (1 << USICS1) // External clock
		      | (0 << USICS0) // ... on positive edge
		      | (0 << USICLK) // ...
		      | (0 << USITC)  // Don't toggle CLK pin
		      ;
	}
	
	// Handle incoming address
	twi_state = STATE_ADDR;

	// Clear the USI status
	USISR = (1 << USISIF) // Clear start condition flag
	      | (1 << USIOIF) // Clear overflow condition flag
	      | (1 << USIPF)  // Keep stop condition flag
	      | (1 << USIDC)  // Clear arbitration error flag
	      | 0x00          // Wait for 8 clock cycles
	      ;
}


ISR(USI_OVF_vect) {
	// Dispatch to state handler
	switch (twi_state) {
		case STATE_ADDR: {
			// We received the address (7 bit address and 1 bit R/W flag and
			// must send an ACK bit if the transmitted address belongs to this
			// controller. If another address was received, nothing must be
			// done.

			// Check if received address is our own
			if ((USIDR >> 1) != twi_address) {
				// Got another address - do not acquire bus
				goto reset;
			}
			
			// Check if read or write mode
			if (USIDR & 0b00000001) {
				// Read mode - we are sending data and the master sends ACK bits

				// Call the load handler
				if (twi_loader(TWI_DIRECTION_READ)) {
					// Prepare sending ACK
					USIDR = 0x00;

				} else {
					// Prepare sending NACK
					USIDR = 0xFF;
				}

				// Send a data byte after sending the address ACK
				twi_state = STATE_READ_DATA;

			} else {
				// Write mode - we are receiving data and the master receives
				// ACK bits

				// Call the load handler
				if (twi_loader(TWI_DIRECTION_WRITE)) {
					// Prepare sending ACK
					USIDR = 0x00;

				} else {
					// Prepare sending NACK
					USIDR = 0xFF;
				}

				// Receive a data byte after sending the address ACK
				twi_state = STATE_WRITE_DATA;
			}

			// Set SDA as output
			bit_set(DDRB, DDB0);

			// Clear all interrupt flags except start condition - shift out a single bit
			USISR = (0 << USISIF) // Keep start condition flag
			      | (1 << USIOIF) // Clear overflow condition flag
			      | (1 << USIPF)  // Clear stop condition flag
			      | (1 << USIDC)  // Clear arbitration error flag
			      | 0x0E          // Wait for a single clock cycle
			      ;

			break;
		}

		case STATE_READ_DATA: {
			// The previous byte (previous data byte or address) transmission was
			// acknowledget (or not) and we must check if transmission was
			// sucessfull and load the next byte for transmission.

			// Check if last byte was not ACKed or we have nothing left to
			// transmit
			if (USIDR != 0x00) {
				// Transmission failed
				goto reset;
			}

			// Set data to send

			if (!twi_reader(&USIDR)) {
				// End of data
				goto reset;
			}

			// Set SDA as output
			bit_set(DDRB, DDB0);
			
			// Receiving ACK after sending the byte
			twi_state = STATE_READ_ACK;

			// Clear all interrupt flags except start condition - shift out 8 bit
			USISR = (0 << USISIF) // Keep start condition flag
			      | (1 << USIOIF) // Clear overflow condition flag
			      | (1 << USIPF)  // Clear stop condition flag
			      | (1 << USIDC)  // Clear arbitration error flag
		  	      | 0x00          // Wait for 8 clock cycles
			      ;

			break;
		}

		case STATE_READ_ACK: {
			// The last byte was transmitted and we have to receive an
			// acknowledge bit from master.

			// Set SDA as input
			bit_clr(DDRB, DDB0);

			// Prepare read
			USIDR = 0x00;
			
			// Send next byte after receiving ACK
			twi_state = STATE_READ_DATA;

			// Clear all interrupt flags except start condition - shift out a single bit
			USISR = (0 << USISIF) // Keep start condition flag
			      | (1 << USIOIF) // Clear overflow condition flag
			      | (1 << USIPF)  // Clear stop condition flag
			      | (1 << USIDC)  // Clear arbitration error flag
			      | 0x0E          // Wait for a single clock cycle
			      ;

			break;
		}

		case STATE_WRITE_DATA: {
			// Master is about to send data

			// Check if last byte was ACKed
			if (USIDR != 0x00) {
				// Transmission failed
				goto reset;
			}

			// Set SDA as input
			bit_clr(DDRB, DDB0);

			// Sending ACK after receiving the byte
			twi_state = STATE_WRITE_ACK;

			// Clear all interrupt flags except start condition - shift in 8 bit
			USISR = (0 << USISIF) // Keep start condition flag
			      | (1 << USIOIF) // Clear overflow condition flag
			      | (1 << USIPF)  // Clear stop condition flag
			      | (1 << USIDC)  // Clear arbitration error flag
		  	      | 0x00          // Wait for 8 clock cycles
			      ;

			break;
		}

		case STATE_WRITE_ACK: {
			// Master has send data - reply with ACK / NACK accordinly
			
			// Handle incoming data
			if (! twi_writer(&USIDR)) {
				// End of data - prepare sending NACK
				USIDR = 0xFF;

			} else {
				// Prepare sending ACK
				USIDR = 0x00;
			}

			// Set SDA as output
			bit_set(DDRB, DDB0);

			twi_state = STATE_WRITE_DATA;

			// Clear all interrupt flags except start condition - shift out a single bit
			USISR = (0 << USISIF) // Keep start condition flag
			      | (1 << USIOIF) // Clear overflow condition flag
			      | (1 << USIPF)  // Clear stop condition flag
			      | (1 << USIDC)  // Clear arbitration error flag
			      | 0x0E          // Wait for a single clock cycle
			      ;

			break;
		}
	}

	return;

reset:
	// Set SDA as input
	bit_clr(DDRB, DDB0);
	
	// Reset communication mode
	USICR = (1 << USISIE) // Enable start condition interrupt
	      | (0 << USIOIE) // Disable overflow interrupt
	      | (1 << USIWM1) // Two-wire mode
	      | (0 << USIWM0) // ... hold SCL low on start condition
	      | (1 << USICS1) // External clock
	      | (0 << USICS0) // ... on positive edge
	      | (0 << USICLK) // ...
	      | (0 << USITC)  // Don't toggle CLK pin
	      ;

	// Clear all interrupt flags except start condition
	USISR = (0 << USISIF) // Keep start condition flag
	      | (1 << USIOIF) // Clear overflow condition flag
	      | (1 << USIPF)  // Clear stop condition flag
	      | (1 << USIDC)  // Clear arbitration error flag
	      | 0x00          // Wait for 8 clock cycles
	      ;
}

