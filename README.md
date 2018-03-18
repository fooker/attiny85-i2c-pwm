# ATTiny85 - I²C - PWM
This is a firmware for ATTiny85 microcontrollers providing a PWM signal controlled via I²C.
The microcontroller acts as a I²C slave and allows a single byte write operation which is used unmodified as PWM output value.


## Pins and Wiring

Beside the usual `VCC` and`GND` pins, the following pins are used:
* 5 (`PB0`) - I²C SDA
* 6 (`PB1`) - PWM output
* 7 (`PB2`) - I²C SCL

The I²C pins utilize the internal pull-ups but external pull-up resistors should be added according to the specifications.


## Configuration

The configuration can be done by mondifying `config.h` accordin to the needs.
Right now, there are only two options to set:

* `TWI_ADDRESS` - Contains the address of the slave on the I²C bus. This must only contain the leading 7 bit of the address without the R/W flag.
* `PWM_PRESCALER` - The prescaler to use for the PWM output. See Table 12-5 in the ATTiny85 datasheed vor valid values. All settings are relative to 8MHz CPU frequence.

