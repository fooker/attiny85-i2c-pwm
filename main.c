#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include <limits.h>
#include <stdbool.h>

#include "config.h"
#include "bits.h"
#include "twi.h"

volatile uint8_t 	isr_even_step;
volatile uint32_t 	isr_step_counter;

volatile uint8_t 	isr_status;
volatile uint8_t 	i2c_setup_flag;

volatile uint8_t 	twi_nibbles_comp;
volatile uint32_t 	twi_nibbles_data;

volatile uint8_t	twi_reader_addr;
volatile uint8_t	twi_reader_read;

volatile uint8_t	timer_oscr;
volatile uint8_t	timer_pres;

volatile uint8_t 	M_ENA 	= 0;
volatile uint8_t 	M_DIR 	= 0;
volatile uint8_t 	M_MOD 	= 0;
volatile uint8_t 	M_MOV 	= 0;
volatile uint32_t 	M_STEPS = 0;
volatile uint8_t 	M_RPM 	= 0;
volatile uint16_t 	M_SPT 	= 800;


#define disable_driver()    bit_set(PORTB, ENA_PIN)
#define enable_driver()     bit_clr(PORTB, ENA_PIN)

inline void driver_test();

// ISR FUNCTIONS TO COUNT STEPS (TODO)
inline void enable_isr();
inline void disable_isr();

// Timer setup functions
void setup_timer(uint8_t rpm, uint16_t spt);
void start_timer();
void stop_timer();

// I2C Callback functions
bool twi_loader(twi_direction_t direction __attribute__((unused)));
bool twi_reader(volatile uint8_t* b);
bool twi_writer(volatile uint8_t* const b);


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

	twi_nibbles_comp = 0;
	twi_nibbles_data = 0;	

	// Go go, interrupts go!
	i2c_setup_flag = 0;
	sei();

	driver_test();

	// Main control loop
	while (true) {
		if (i2c_setup_flag > 0) {
			if (bit_get(i2c_setup_flag, 4)) {
				if (M_SPT) setup_timer(M_RPM, M_SPT);			
			}
			if (bit_get(i2c_setup_flag, 0)) {
				(M_ENA) 	? enable_driver()		 		: disable_driver();
				(M_DIR)		? bit_set(PORTB, DIR_PIN) 		: bit_clr(PORTB, DIR_PIN);
				(M_MOD)		? enable_isr() 					: disable_isr();
				(M_MOV)		? start_timer() 				: stop_timer();
			}
			i2c_setup_flag = 0;
		}
	}

	return (1);
}




inline void driver_test() {
	M_ENA = 1;
	M_DIR = 1;
	M_MOV = 1;
	M_MOD = 1;
	bit_set(i2c_setup_flag,0);

	M_STEPS = 800;
	bit_set(i2c_setup_flag,1);

	// M_PRES = 0b0110;		// 1 seg
	// M_OSCR = 0b10011011;	// 1 seg
	// bit_set(i2c_setup_flag,2);	

	M_RPM = 6;
	M_SPT = 800;
	bit_set(i2c_setup_flag, 4);	
}


// ISR FUNCTIONS TO COUNT STEPS (TODO)
void enable_isr() {
//    isr_status = 0;
	cli();
	isr_even_step = 0;
	isr_step_counter = M_STEPS;
	TIMSK |= (1 << TOIE1);
	TIMSK |= (1 << OCIE1A);
	sei();
}

void disable_isr() {
	cli();
	TIMSK &= ~(1 << TOIE1);
	TIMSK &= ~(1 << OCIE1A);
	sei();
}




inline void setup_timer(uint8_t rpm, uint16_t spt) {
	uint8_t _pre, _ocr;	
	if ((rpm == 0) || (spt == 0)) {
		_pre = _ocr = 0;
	} else {		
		uint16_t comp = (F_CPU * 60UL / 256UL) * spt * rpm;

		_pre = 1;
		uint32_t k;
		for (k=2; _pre<16; _pre++) {
			if (comp < k) {
				break;
			} else {
				k*=2;
			}
		}
		_ocr = ( (F_CPU * 60UL) / (k * spt * rpm) ) - 1;
	}
	
	timer_oscr = _ocr;
	timer_pres = _pre;
}

inline void start_timer() {
	cli();
	TCNT1 = 0;
	OCR1A = timer_oscr;
	OCR1C = timer_oscr;
	TCCR1 = (1 << CTC1)            // Clear on OCRA1
	      | (0 << PWM1A)           // Disable Modulator A
	      | (0 << COM1A1)          // Toggle OC1 (Pin)
	      | (1 << COM1A0)          // ... on compare match
	      | (timer_pres & 0x0F)	   // Set prescaler
	      ;
	sei();
}

inline void stop_timer() {
	cli();
	TCCR1 = (1 << CTC1);			// Stop timer, disconnect OC1 (Pin)
	bit_clr(PORTB, PUL_PIN); 		// Set OC1 OFF
	sei();
}







bool twi_writer(volatile uint8_t* const b) {
	uint8_t	func = *b >> 4;			// |*F3*|*F4*|*F1*|*F0*| D3 | D2 | D1 | D0 |
	uint8_t data = *b & 0x0F;

	switch(func) {
		case FUNC_SHAFT:
			M_ENA 			= bit_get(data, REG_ENA);
			M_DIR 			= bit_get(data, REG_DIR);
			M_MOV			= bit_get(data, REG_MOV);
			M_MOD			= bit_get(data, REG_MOD);
			bit_set(i2c_setup_flag, 0);
			twi_nibbles_comp = 0;
			break;
		
		case FUNC_RPML:
		case FUNC_RPMH:
			twi_nibbles_comp |= 1<<(func-FUNC_RPML);
			twi_nibbles_data |= ((data ) << (4*(func-FUNC_RPML)));			
			if (func == FUNC_RPMH) {
				if (twi_nibbles_comp == 0b00000011) {
					M_RPM = twi_nibbles_data;	
					bit_set(i2c_setup_flag, 4);
				}
				twi_nibbles_comp = 0;
				twi_nibbles_data = 0;	 
			}
			break;
		
		case FUNC_RES_0:	//reserved
			break;
			
		// case FUNC_PRESC:
		// 	M_PRES = data;			
		// 	bit_set(i2c_setup_flag, 1);
		// 	twi_nibbles_comp = 0;
		// 	break;

		// case FUNC_OCRAL:
		// case FUNC_OCRAH:
		// 	twi_nibbles_comp |= 1<<(func-FUNC_OCRAL);
		// 	twi_nibbles_data |= (data << (4*(func-FUNC_OCRAL)));			
		// 	if (func == FUNC_OCRAH) {
		// 		if (twi_nibbles_comp == 0b00000011) {
		// 			M_OSCR = (uint8_t) twi_nibbles_data;
		// 			bit_set(i2c_setup_flag, 1);	
		// 		}	
		// 		twi_nibbles_comp = 0;
		// 		twi_nibbles_data = 0;
		// 	}
		// 	break;
			
		case FUNC_PPT0L:
		case FUNC_PPT0H:
		case FUNC_PPT1L:
		case FUNC_PPT1H:
		case FUNC_PPT2L:
		case FUNC_PPT2H:
		case FUNC_PPT3L:
		case FUNC_PPT3H:
			twi_nibbles_comp |= 1<<(func-FUNC_PPT0L);
			twi_nibbles_data |= ((data ) << (4*(func-FUNC_PPT0L)));			
			if (func == FUNC_PPT3H) {
				if (twi_nibbles_comp == 0b11111111) {
					M_STEPS = twi_nibbles_data;	
					bit_set(i2c_setup_flag, 2);
				}
				twi_nibbles_comp = 0;
				twi_nibbles_data = 0;	 
			}
			break;

		case FUNC_SPT0L:
		case FUNC_SPT0H:
		case FUNC_SPT1L:
		case FUNC_SPT1H:
			twi_nibbles_comp |= 1<<(func-FUNC_SPT0L);
			twi_nibbles_data |= ((data ) << (4*(func-FUNC_SPT0L)));			
			if (func == FUNC_SPT1H) {
				if (twi_nibbles_comp == 0b00001111) {
					M_SPT = twi_nibbles_data;	
					bit_set(i2c_setup_flag, 3);
				}
				twi_nibbles_comp = 0;
				twi_nibbles_data = 0;	 
			}
			break;
		
		
	}

	return true;
}

bool twi_loader(twi_direction_t direction __attribute__((unused))) {
	return true;
}

bool twi_reader(volatile uint8_t* b) {
	// if (twi_reader_addr < FUNC_END) {
	// 	switch(twi_reader_addr) {
	// 		case FUNC_SHAFT:
	// 			*b = (twi_reader_addr<<4) | (M_ENA << REG_ENA) | (M_DIR << REG_DIR) | (M_MOV << REG_MOV) | (M_MOD << REG_MOD);
	// 			break;
	// 		case FUNC_PRESC:
	// 			*b = M_PRES;
	// 			break;
	// 		case FUNC_OCRAL:
	// 			*b = (twi_reader_addr<<4) | ((M_OSCR >> 0) & 0x0F);
	// 			break;
	// 		case FUNC_OCRAH:
	// 			*b = (twi_reader_addr<<4) | ((M_OSCR >> 4) & 0x0F);
	// 			break;
	// 		case FUNC_PPT0L:
	// 			*b = (twi_reader_addr<<4) | ((M_STEPS >> 0) & 0x0F);
	// 			break;
	// 		case FUNC_PPT0H:
	// 			*b = (twi_reader_addr<<4) | ((M_STEPS >> 4) & 0x0F);
	// 			break;
	// 		case FUNC_PPT1L:
	// 			*b = (twi_reader_addr<<4) | ((M_STEPS >> 8) & 0x0F);
	// 			break;
	// 		case FUNC_PPT1H:
	// 			*b = (twi_reader_addr<<4) | ((M_STEPS >> 12) & 0x0F);
	// 			break;
	// 		case FUNC_PPT2L:
	// 			*b = (twi_reader_addr<<4) | ((M_STEPS >> 16) & 0x0F);
	// 			break;
	// 		case FUNC_PPT2H:
	// 			*b = (twi_reader_addr<<4) | ((M_STEPS >> 20) & 0x0F);
	// 			break;
	// 		case FUNC_PPT3L:	
	// 			*b = (twi_reader_addr<<4) | ((M_STEPS >> 24) & 0x0F);
	// 			break;
	// 		case FUNC_PPT3H:
	// 			*b = (twi_reader_addr<<4) | ((M_STEPS >> 28) & 0x0F);
	// 			break;				
	// 	}
	// 	twi_reader_addr++;
	// } else {
	// 	twi_reader_addr = 0;
	// }
	*b = 0;
	return true;
}








//TIMER 1 COMPARE match A
ISR(TIMER1_COMPA_vect) {
	if (isr_step_counter > 0) {
		if (isr_even_step) isr_step_counter--;
		isr_even_step ^= 1;
	} else {
		stop_timer();
		disable_isr();
		disable_driver();
	}
	
	// ------------------- TEST
	/*
	if (isr_status) {
		isr_status = 0;
		bit_clr(PORTB, DIR_PIN);
	} else {
		isr_status = 1;
		bit_set(PORTB, DIR_PIN);
	}
	*/
	// -------------------
}
