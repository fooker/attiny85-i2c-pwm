#define FOSC 1000000L

// PINOUT
#define ENA_PIN			PB3
#define DIR_PIN			PB4
#define PUL_PIN			PB1

// FUNC SHAFT
#define REG_EN      0
#define REG_DIR     1
#define REG_START   2

// FUNCTION ADDRESS
#define FUNC_SHAFT  0
#define FUNC_PRESC  1
#define FUNC_OCRAL  2
#define FUNC_OCRAH  3

// The I²C address (7 bit) to listen on
static const uint8_t TWI_ADDRESS = 0x20;
