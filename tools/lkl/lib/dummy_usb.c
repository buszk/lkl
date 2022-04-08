// SPDX-License-Identifier: GPL-2.0
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <lkl_host.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/vfio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/eventfd.h>
#include <execinfo.h>
#include "fuzz_input.h"


static const __u8 device_desc[] =
{
	0x12,        // bLength
	0x01,        // bDescriptorType (Device)
	0x00, 0x02,  // bcdUSB 2.00
	0x02,        // bDeviceClass (Communications and CDC Control)
	0x00,        // bDeviceSubClass 
	0x00,        // bDeviceProtocol 
	0x40,        // bMaxPacketSize0 64
	0x18, 0x16,  // idVendor 0x1618
	0x13, 0x91,  // idProduct 0x9113
	0x00, 0x00,  // bcdDevice 0.00
	0x01,        // iManufacturer (String Index)
	0x02,        // iProduct (String Index)
	0x0A,        // iSerialNumber (String Index)
	0x01,        // bNumConfigurations 1
};


static const __u8 config_desc[] =
{
	0x09,        // bLength
	0x02,        // bDescriptorType (USB_DT_CONFIG)
	0x27, 0x00,  // wTotalLength 39
	0x01,        // bNumInterfaces 1
	0x02,        // bConfigurationValue
	0x09,        // iConfiguration (String Index)
	0xC0,        // bmAttributes Self Powered
	// 0x32,        // bMaxPower 100mA
	0x00,        // bMaxPower 0mA

	0x09,        // bLength
	0x04,        // bDescriptorType (Interface)
	0x00,        // bInterfaceNumber 0
	0x00,        // bAlternateSetting
	0x03,        // bNumEndpoints 3
	0x02,        // bInterfaceClass
	0x02,        // bInterfaceSubClass
	0xFF,        // bInterfaceProtocol
	0x06,        // iInterface (String Index)

	0x07,        // bLength
	0x05,        // bDescriptorType (Endpoint)
	0x81,        // bEndpointAddress (IN/D2H)
	0x02,        // bmAttributes (Bulk)
	0x40, 0x00,  // wMaxPacketSize 64
	0x00,        // bInterval 0 (unit depends on device speed)

	0x07,        // bLength
	0x05,        // bDescriptorType (Endpoint)
	0x01,        // bEndpointAddress (OUT/H2D)
	0x02,        // bmAttributes (Bulk)
	0x40, 0x00,  // wMaxPacketSize 64
	0x00,        // bInterval 0 (unit depends on device speed)

	0x07,        // bLength
	0x05,        // bDescriptorType (Endpoint)
	0x82,        // bEndpointAddress (IN/D2H)
	0x02,        // bmAttributes (Bulk)
	0x40, 0x00,  // wMaxPacketSize 64
	0x00,        // bInterval 0 (unit depends on device speed)
};

uint32_t dummy_get_device_desc(void *dst, size_t s) {
    assert(s <= sizeof(device_desc));
    memcpy(dst, device_desc, s);
    return s;
}

uint32_t dummy_get_config_desc(void *dst, size_t s) {
    assert(s <= sizeof(config_desc));
    memcpy(dst, config_desc, s);
    return s;
}

uint32_t dummy_get_size(void* b, size_t s) {
    get_size(b, s);
    return s;
}

uint8_t dummy_get_control_byte(void) {
    return get_byte();
    // return 0;
}

struct lkl_dev_usb_ops fuzz_usb_ops = {
    .get_data = dummy_get_size,
    .get_control_byte = dummy_get_control_byte,
    .get_device_desc = dummy_get_device_desc,
    .get_config_desc = dummy_get_config_desc,
};
