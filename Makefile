CFLAGS+=-DF_CPU=16500000 -Wall -Os -Iusbdrv -mmcu=attiny85 --param=min-pagesize=0
FW_OBJS=firmware.o usbdrv/usbdrv.o usbdrv/oddebug.o usbdrv/usbdrvasm.o

all: usb_led firmware.hex

usb_led: driver.c
	gcc -o usb_led driver.c -lusb

firmware.hex: firmware.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

firmware.elf: $(FW_OBJS)
	avr-gcc $(CFLAGS) $(FW_OBJS) -o $@

%.o: %.c usbdrv/usbconfig.h
	avr-gcc $(CFLAGS) -c $< -o $@

%.o: %.S
	avr-gcc $(CFLAGS) -x assembler-with-cpp -c $< -o $@

flash: firmware.hex
	avrdude -c usbtiny -p t85 -U flash:w:firmware.hex:a

dump_program:
	avrdude -c usbtiny -p t85 -U flash:r:dump.hex:r

fuse:
	avrdude -c usbtiny -p t85 -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m

clean:
	rm -rf $(FW_OBJS) *.hex *.elf usb_led
