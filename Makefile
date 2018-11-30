SOURCES = \
	main.c \
	twi.c

TARGET = firmware

OBJECTS = $(SOURCES:%.c=%.o)

CFLAGS = -mmcu=attiny85 \
	 -DF_CPU=8000000UL \
	 -Os \
	 -funsigned-char \
	 -funsigned-bitfields \
	 -fpack-struct \
	 -fshort-enums \
	 -fno-unit-at-a-time \
	 -Wall \
	 -Wextra \
	 -Werror \
	 -std=gnu11 

LDFLAGS = -Wl,-Map=$(TARGET).map,--cref

#AVRDUDE_PROGRAMMER = stk500v2
AVRDUDE_PROGRAMMER = avrisp -b 19200
AVRDUDE_PORT = /dev/ttyACM0

CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
NM = avr-nm
AVRDUDE = avrdude


all: $(TARGET).hex $(TARGET).lss $(TARGET).sym size

flash: all
	$(AVRDUDE) -p attiny85 -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER) -U flash:w:$(TARGET).hex -U lfuse:w:0xE2:m -U hfuse:w:0xDF:m -U efuse:w:0xFF:m

%.hex: %.elf
	$(OBJCOPY) -O ihex -j .data -j .text $< $@

%.lss: %.elf
	$(OBJDUMP) -h -S $< > $@

%.sym: %.elf
	$(NM) -n $< > $@

%.elf: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

size: $(TARGET).elf
	avr-size --mcu=attiny85 -C $(TARGET).elf

clean:
	$(RM) $(TARGET).elf
	$(RM) $(TARGET).hex
	$(RM) $(TARGET).lss
	$(RM) $(TARGET).sym
	$(RM) $(OBJECTS)

.PHONY: all flash size clean

