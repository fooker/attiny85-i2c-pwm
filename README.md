# ATTiny85 I²C interface for TB6600
This firmware is based on the attiny85-i2c-pwm made by fooker (https://github.com/fooker/attiny85-i2c-pwm)
The idea is to implement a microcontroller interface (I2C slave) and signal generator to command an TB6600 or similar stepper motor driver.
New micros like ESP8266 don't work very well as signal generators, but provide some other functionalities. So with this code you can use an ATTiny85 as an interface to communicate to the TB6600 and setup some parameters like RPM, direction, etc.

## TB6600 pins and info

The TB6600 uses 3 opto-coupled signals to command the stepper motor: pulse (PUL+ PUL-), direction (DIR+ DIR-) and enable (ENA+ ENA-).
More information at: https://www.dfrobot.com/wiki/index.php/TB6600_Stepper_Motor_Driver_SKU:_DRI0043

## Pins and Wiring

Beside the usual `VCC` and`GND` pins, the following pins are used:
* 2 (`PB0`) - ENA+
* 3 (`PB0`) - DIR+
* 5 (`PB0`) - I²C SDA
* 6 (`PB1`) - PUL+
* 7 (`PB2`) - I²C SCL

The I²C pins utilize the internal pull-ups but external pull-up resistors should be added according to the specifications.

## Configuration

The configuration can be done by mondifying `config.h` accordin to the needs.

* `TWI_ADDRESS` - Contains the address of the slave on the I²C bus. This must only contain the leading 7 bit of the address without the R/W flag.
