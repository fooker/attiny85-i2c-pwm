#ifndef TWI_H_
#define TWI_H_


#include <stdint.h>
#include <stdbool.h>


typedef uint8_t twi_address_t;

typedef enum {
	TWI_DIRECTION_WRITE = 0,
	TWI_DIRECTION_READ = 1
} twi_direction_t;

typedef bool (*twi_loader_t)(twi_direction_t);
typedef bool (*twi_reader_t)(volatile uint8_t* b);
typedef bool (*twi_writer_t)(volatile uint8_t* const b);



/**
 * Initialize the USI as TWI slave.
 *
 * @param address: the address of the slave
 */
void twi_init(twi_address_t address,
			  twi_loader_t loader,
			  twi_reader_t reader,
			  twi_writer_t writer);


#endif /* TWI_H_ */

