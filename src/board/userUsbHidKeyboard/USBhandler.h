/*
 * USBhandler.h — USB interrupt handler for composite HID device
 */

#ifndef __USB_HANDLER_H__
#define __USB_HANDLER_H__

// clang-format off
#include <stdint.h>
#include "include/ch5xx.h"
#include "include/ch5xx_usb.h"
#include "USBconstant.h"
// clang-format on

// clang-format off
extern __xdata __at (EP0_ADDR) uint8_t Ep0Buffer[];
extern __xdata __at (EP1_ADDR) uint8_t Ep1Buffer[];
extern __xdata __at (EP2_ADDR) uint8_t Ep2Buffer[];
// clang-format on

extern __data uint16_t SetupLen;
extern __data uint8_t SetupReq;
volatile extern __xdata uint8_t UsbConfig;

extern const __code uint8_t *__data pDescr;

void USB_EP1_IN(void);
void USB_EP1_OUT(void);

#define UsbSetupBuf ((PUSB_SETUP_REQ)Ep0Buffer)

/* ---- Endpoint callback routing ---- */

/* OUT */
#define EP0_OUT_Callback  USB_EP0_OUT
#define EP1_OUT_Callback  NOP_Process   /* EP1 is TX-only */
#define EP2_OUT_Callback  NOP_Process   /* EP2 is TX-only */
#define EP3_OUT_Callback  NOP_Process
#define EP4_OUT_Callback  NOP_Process

/* SOF */
#define EP0_SOF_Callback  NOP_Process
#define EP1_SOF_Callback  NOP_Process
#define EP2_SOF_Callback  NOP_Process
#define EP3_SOF_Callback  NOP_Process
#define EP4_SOF_Callback  NOP_Process

/* IN */
#define EP0_IN_Callback   USB_EP0_IN
#define EP1_IN_Callback   USB_EP1_IN
#define EP2_IN_Callback   NOP_Process   /* EP2 always NAK */
#define EP3_IN_Callback   NOP_Process
#define EP4_IN_Callback   NOP_Process

/* SETUP */
#define EP0_SETUP_Callback USB_EP0_SETUP
#define EP1_SETUP_Callback NOP_Process
#define EP2_SETUP_Callback NOP_Process
#define EP3_SETUP_Callback NOP_Process
#define EP4_SETUP_Callback NOP_Process

void USBInterrupt(void);
void USBDeviceCfg(void);
void USBDeviceIntCfg(void);
void USBDeviceEndPointCfg(void);

/* Flag set by USB ISR when bootloader command is received.
 * Checked in main loop() — deferred so EP0 STATUS stage completes first. */
extern volatile __xdata uint8_t bootloader_request;

/* Jump to CH552 USB bootloader (call from main context, NOT from ISR) */
void enterBootloader(void);

#endif
