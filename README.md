# ATTiny85 I²C interface for TB6600
This firmware is based on the attiny85-i2c-pwm made by fooker (https://github.com/fooker/attiny85-i2c-pwm)

The idea is to implement a microcontroller interface (I2C slave) and signal generator to command an TB6600 or similar stepper motor driver.

New micros like ESP8266 don't work very well as signal generators, but provide some other functionalities. So with this code you can use an ATTiny85 as an interface to communicate to the TB6600 and setup some parameters like RPM, direction, etc just writting on function registers via I²C.

## TB6600 pins and info

The TB6600 uses 3 opto-coupled signals to command the stepper motor: pulse (`PUL+`, `PUL-`), direction (`DIR+`, `DIR-`) and enable (`ENA+`, `ENA-`).
More information at: 
https://www.dfrobot.com/wiki/index.php/TB6600_Stepper_Motor_Driver_SKU:_DRI0043

## Pins and Wiring

Beside the usual `VCC` and`GND` pins, the following pins are used:
* 2 (`PB3`) - ENA+
* 3 (`PB4`) - DIR+
* 5 (`PB0`) - I²C SDA
* 6 (`PB1`) - PUL+
* 7 (`PB2`) - I²C SCL

The I²C pins utilize the internal pull-ups but external pull-up resistors should be added according to the specifications.

## Configuration

The configuration can be done by mondifying `config.h` accordin to the needs.

* `TWI_ADDRESS` - Contains the address of the slave on the I²C bus. This must only contain the leading 7 bit of the address without the R/W flag.
* `ENA_PIN` - Port to command the ENA+ signal
* `DIR_PIN` - Same for the DIR+ signal
* `PUL_PIN` - Same for the PUL+ signal

## Registers

To setup the ATTiny85 just write over the I2C the function address (higher nibble) and the data to store (lower nibble) according to this table:

| Higher nibble | Lower nibble |
|---------------|--------------|
| F[3-0]        | D[3-0]       |
 
| F3 | F2 | F1 | F0 | D3 | D2 | D1 | D0 | Register function                            |
|----|----|----|----|----|----|----|----|----------------------------------------------|
|  0 |  0 |  0 |  0 | X  | X  |DIR |ENA | `FUNC_SHAFT`: Driver setup signals           |
|  0 |  0 |  0 |  1 | X  | X  | X  | X  | `FUNC_PRESC`: Timer1 prescaler               |
|  0 |  0 |  1 |  0 | X  | X  | X  | X  | `FUNC_OCRAL`: Timer1 counter (Lower nibble)  |
|  0 |  0 |  1 |  1 | X  | X  | X  | X  | `FUNC_OCRAH`: Timer1 counter (Higher nibble) |

### Function example to setup RPM

This is a draft function to represent how to set the ATTiny prescale and counter from a RPM speed value.

```c++
#define 	MIN_RPM		0
#define 	MAX_RPM		250

uint16_t	_ppv 	= 800		//800 pulses to make 1 turn

void setRPM(uint16_t rpm) {
  if ( (rpm<MIN_RPM) || (rpm>MAX_RPM) ) return;
  
  uint8_t		_pre;		// prescale value (0 to 15)
  uint8_t		_ocr;		// counter value (0 to 255)

  if (rpm == 0) {
    _pre = 0;
    _ocr = 0;
  } else {    
    uint16_t comp = (8000000L * 60) / (256 * _ppv * rpm);

    _pre = 1;
    uint16_t k;
    for (k=2; _pre<16; _pre++) {
      if (comp < k) {
        break;
      } else {
        k<<=1;
      }
    }

    _ocr = ((8000000L * 60) / (k * _ppv * rpm)) - 1;
    
    i2c_write(FUNC_PRESC, pre & 0x0F);
    i2c_write(FUNC_OCRAL, ocr & 0x0F);
    i2c_write(FUNC_OCRAH, (ocr >> 4) & 0x0F);
  }
```

## TODO:
* Polish the code.
* Function to move the shaft an specified number of steps. This probably will require to add more function registers in the future.
* Add an Arduino library to make it work with other micros like esp8266.
