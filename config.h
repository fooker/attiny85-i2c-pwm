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
#define FUNC_SHAFT  		0        // SHAFT
#define FUNC_RPML 			1        // Revolutions-per-Minute
#define FUNC_RPMH 			2
#define FUNC_RES_0          3   
#define FUNC_PPT0L			4        // Total steps on command
#define FUNC_PPT0H  		5
#define FUNC_PPT1L  		6
#define FUNC_PPT1H  		7
#define FUNC_PPT2L  		8
#define FUNC_PPT2H  		9
#define FUNC_PPT3L  		10
#define FUNC_PPT3H			11
#define FUNC_SPT0L 			12        // Steps-per-Turn
#define FUNC_SPT0H 			13
#define FUNC_SPT1L 			14
#define FUNC_SPT1H 			15

#define FUNC_END			16


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
