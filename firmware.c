#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>

#include <usbdrv.h>
#include <util/delay.h>


#define USB_LED_OFF 0
#define USB_LED_ON  1
#define abs(x) ((x) > 0 ? (x) : (-x))
#define LED_PIN (1<<PB3)


USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data;

    switch(rq->bRequest) { // custom command is in the bRequest field
        case USB_LED_ON:
            PORTB |= LED_PIN;
            break;
        case USB_LED_OFF:
            PORTB &= ~LED_PIN;
            break;
    }

    return 0;
}


// Oscillator calibration routine, called by V-USB after device reset
void hadUsbReset() {
    int frameLength, targetLength = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);
    int bestDeviation = 9999;
    uchar trialCal, bestCal=0, step, region;

    // do a binary search in regions 0-127 and 128-255 to get optimum OSCCAL
    for(region = 0; region <= 1; region++) {
        frameLength = 0;
        trialCal = (region == 0) ? 0 : 128;

        for(step = 64; step > 0; step >>= 1) {
            if(frameLength < targetLength) // true for initial iteration
                trialCal += step; // frequency too low
            else
                trialCal -= step; // frequency too high

            OSCCAL = trialCal;
            frameLength = usbMeasureFrameLength();

            if(abs(frameLength-targetLength) < bestDeviation) {
                bestCal = trialCal; // new optimum found
                bestDeviation = abs(frameLength -targetLength);
            }
        }
    }

    OSCCAL = bestCal;
}

/*
 * Re-enumeration is enforced to grant that both host and device agree on the
 * same USB identifier (the device could reboot, change the identifier, and the
 * host would not know).
 */
void usb_setup()
{
    uchar i;

    usbInit();
    usbDeviceDisconnect(); // enforce re-enumeration
    for(i = 0; i<250; i++) { // wait 500 ms
        wdt_reset(); // keep the watchdog happy
        _delay_ms(2);
    }
    usbDeviceConnect();
}

int main()
{
    DDRB = LED_PIN; // PB0 as output
    wdt_enable(WDTO_1S); // enable 1s watchdog timer
    usb_setup();
    sei(); // Enable interrupts after re-enumeration

    while(1) {
        wdt_reset(); // keep the watchdog happy
        usbPoll();
    }

    return 0;
}
