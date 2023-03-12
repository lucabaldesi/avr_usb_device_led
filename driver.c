#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>

#define OBDEV_VENDOR_ID 0x16c0
#define OBDEV_DEVICE_ID 0x05dc
#define USB_ENG_LANG 0x0409
#define USB_LED_OFF 0
#define USB_LED_ON  1


int usbGetDescriptorString(usb_dev_handle *dev, int index, int langid,
                           char *buf, int buflen) {
    char buffer[256];
    int rval, i;

    // make standard request GET_DESCRIPTOR, type string and given index
    // (e.g. dev->iProduct)
    rval = usb_control_msg(dev,
        USB_TYPE_STANDARD | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
        USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid,
        buffer, sizeof(buffer), 1000);

    if(rval < 0) // error
        return rval;

    // rval should be bytes read, but buffer[0] contains the actual response size
    if((unsigned char)buffer[0] < rval)
        rval = (unsigned char)buffer[0]; // string is shorter than bytes read

    if(buffer[1] != USB_DT_STRING) // second byte is the data type
        return 0; // invalid return type

    // we're dealing with UTF-16LE here so actual chars is half of rval,
    // and index 0 doesn't count
    rval /= 2;

    /* lossy conversion to ISO Latin1 */
    for(i = 1; i < rval && i < buflen; i++) {
        if(buffer[2 * i + 1] == 0)
            buf[i-1] = buffer[2 * i];
        else
            buf[i-1] = '?'; /* outside of ISO Latin1 range */
    }
    buf[i-1] = 0;

    return i-1;
}


usb_dev_handle * usbOpenDevice(int vendor, char *vendorName, 
                                      int product, char *productName) {
    struct usb_bus *bus;
    struct usb_device *dev;
    char devVendor[256], devProduct[256];
    int res;

    usb_dev_handle * handle = NULL;

    usb_init();
    usb_find_busses();
    usb_find_devices();

    bus=usb_get_busses();
    while (bus && handle == NULL) {
        dev = bus->devices;
        while (dev && handle == NULL) {
            if (dev->descriptor.idVendor == vendor &&
                dev->descriptor.idProduct == product) {
                handle = usb_open(dev);
                if (handle) {
                    res = usbGetDescriptorString(handle, dev->descriptor.iManufacturer, USB_ENG_LANG, devVendor, sizeof(devVendor));
                    if (res >= 0) {
                        res = usbGetDescriptorString(handle, dev->descriptor.iProduct, USB_ENG_LANG, devProduct, sizeof(devProduct));
                        if (res < 0)
                            fprintf(stderr, "Warning: cannot query product for device: %s\n",
                                    usb_strerror());
                    } else
                        fprintf(stderr, "Warning: cannot query manufacturer for device: %s\n",
                                usb_strerror());
                    if (res < 0 || strcmp(devVendor, vendorName) != 0 || strcmp(devProduct, productName) != 0 ) {
                        usb_close(handle);
                        handle = NULL;
                    }
                } else {
                    fprintf(stderr, "Warning: cannot open USB device: %s\n",
                            usb_strerror());
                }
            }
            dev = dev->next;
        }
        bus = bus->next;
    }
    return handle;
}


int main(int argc, char **argv) {
    usb_dev_handle *handle = NULL;
    int nBytes = 0;
    char buffer[256];

    if(argc < 2) {
        printf("Usage:\n");
        printf("%s on\n", argv[0]);
        printf("%s off\n", argv[0]);
        exit(1);
    }

    handle = usbOpenDevice(OBDEV_VENDOR_ID, "baldesi.ovh", OBDEV_DEVICE_ID, "usbled");

    if(handle == NULL) {
        fprintf(stderr, "Could not find USB device!\n");
        exit(1);
    }

    if(strcmp(argv[1], "on") == 0) {
        nBytes = usb_control_msg(handle,
            USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
            USB_LED_ON, 0, 0, (char *)buffer, sizeof(buffer), 5000);
    } else if(strcmp(argv[1], "off") == 0) {
        nBytes = usb_control_msg(handle,
            USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
            USB_LED_OFF, 0, 0, (char *)buffer, sizeof(buffer), 5000);
    }

    if(nBytes < 0)
        fprintf(stderr, "USB error: %s\n", usb_strerror());

    usb_close(handle);

    return 0;
}
