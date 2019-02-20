#define FOSC 1000000L

// PINOUT
#define ENA_PIN					PB3
#define DIR_PIN					PB4
#define PUL_PIN					PB1

// FUNC SHAFT
#define REG_ENA						0
#define REG_DIR						1
#define REG_MOV						2
#define REG_MOD						3

// FUNCTION ADDRESS
#define FUNC_SHAFT  		0x00        // SHAFT
#define FUNC_PRESC  		0x01        // PRESCALER
#define FUNC_OCRAL  		0x02        // OCRA
#define FUNC_OCRAH 			0x03
#define FUNC_PPT0L			0x04        // STEPS
#define FUNC_PPT0H  		0x05
#define FUNC_PPT1L  		0x06
#define FUNC_PPT1H  		0x07
#define FUNC_PPT2L  		0x08
#define FUNC_PPT2H  		0x09
#define FUNC_PPT3L  		0x0A
#define FUNC_PPT3H			0x0B
#define FUNC_END			0x0C


// MODE
#define MODE_FREERUN			0
#define MODE_STEPS				1

// DRIVER
#define DRIVER_ENABLE			1
#define DRIVER_DISABLE  		0

// DIRECTION
#define CWS_DIR     1   //Clock-wise
#define CCW_DIR     0   //Counter Clock-wise



// The I²C address (7 bit) to listen on
static const uint8_t TWI_ADDRESS = 0x20;
