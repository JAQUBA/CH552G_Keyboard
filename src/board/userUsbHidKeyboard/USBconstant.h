/*
 * USBconstant.h — USB descriptors for composite HID device
 *   Interface 0: HID Keyboard
 *   Interface 1: HID Vendor (configuration via Feature Reports)
 */

#ifndef __USB_CONST_DATA_H__
#define __USB_CONST_DATA_H__

// clang-format off
#include <stdint.h>
#include "include/ch5xx.h"
#include "include/ch5xx_usb.h"
#include "usbCommonDescriptors/StdDescriptors.h"
#include "usbCommonDescriptors/HIDClassCommon.h"
// clang-format on

/* Endpoint addresses in USB RAM
 * Override board-level defaults (-DEP0_ADDR=0 -DEP1_ADDR=10 -DEP2_ADDR=20)
 * because our composite device uses a different layout:
 *   EP0  8 B @ 0     (bidirectional control)
 *   EP1 64 B @ 10    (TX-only, keyboard IN)
 *   EP2 64 B @ 74    (TX-only, vendor  IN)
 */
#undef EP0_ADDR
#undef EP1_ADDR
#undef EP2_ADDR
#define EP0_ADDR  0
#define EP1_ADDR  10
#define EP2_ADDR  74

/* Interface 0: Keyboard */
#define KEYBOARD_EPADDR  0x81
#define KEYBOARD_EPSIZE  8

/* Interface 1: Vendor config */
#define VENDOR_EPADDR    0x82
#define VENDOR_EPSIZE    8

/** Composite configuration descriptor structure. */
typedef struct {
  USB_Descriptor_Configuration_Header_t Config;

  /* ---- Interface 0: HID Keyboard ---- */
  USB_Descriptor_Interface_t HID_KeyboardInterface;
  USB_HID_Descriptor_HID_t  HID_KeyboardHID;
  USB_Descriptor_Endpoint_t HID_KeyboardINEndpoint;

  /* ---- Interface 1: HID Vendor (config) ---- */
  USB_Descriptor_Interface_t HID_VendorInterface;
  USB_HID_Descriptor_HID_t  HID_VendorHID;
  USB_Descriptor_Endpoint_t HID_VendorINEndpoint;
} USB_Descriptor_Configuration_t;

extern __code USB_Descriptor_Device_t DeviceDescriptor;
extern __code USB_Descriptor_Configuration_t ConfigurationDescriptor;
extern __code uint8_t KeyboardReportDescriptor[];
extern __code uint8_t VendorReportDescriptor[];
extern __code uint8_t LanguageDescriptor[];
extern __code uint16_t SerialDescriptor[];
extern __code uint16_t ProductDescriptor[];
extern __code uint16_t ManufacturerDescriptor[];

#endif
