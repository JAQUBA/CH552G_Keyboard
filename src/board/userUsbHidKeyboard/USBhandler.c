/*
 * USBhandler.c — USB interrupt handler for composite HID device
 *
 * Interface 0: HID Keyboard  (EP1 IN — keystrokes, EP0 — LED SET_REPORT)
 * Interface 1: HID Vendor    (EP2 IN — dummy NAK, EP0 — Feature Reports)
 */

#include "USBhandler.h"
#include "USBconstant.h"
#include "../config.h"
#include "protocol.h"

/* ---- Endpoint buffers -------------------------------------------- */
// clang-format off
__xdata __at (EP0_ADDR) uint8_t Ep0Buffer[8];
__xdata __at (EP1_ADDR) uint8_t Ep1Buffer[64];    /* TX-only (keyboard IN) */
__xdata __at (EP2_ADDR) uint8_t Ep2Buffer[64];    /* TX-only (vendor  IN)  */
// clang-format on

#if (EP2_ADDR + 64) > USER_USB_RAM
#error "USB endpoints exceed available USB RAM. Increase USER_USB_RAM."
#endif

/* ---- Shared state ------------------------------------------------ */
__data uint16_t SetupLen;
__data uint8_t  SetupReq;
volatile __xdata uint8_t UsbConfig;

__code uint8_t *__data pDescr;

volatile uint8_t usbMsgFlags = 0;
volatile __xdata uint8_t bootloader_request = 0;

__xdata uint8_t keyboardProtocol = 1;   /* 0=boot  1=report */
__xdata uint8_t keyboardLedStatus = 0;

/* State saved during SETUP for use in EP0_OUT */
static __data uint8_t setReportIface  = 0;
static __data uint8_t setReportType   = 0;
static __data uint8_t setReportId     = 0;

/* ---- Helpers ----------------------------------------------------- */
inline void NOP_Process(void) {}

/* ================================================================== */
/*  EP0 SETUP                                                          */
/* ================================================================== */
void USB_EP0_SETUP() {
  __data uint8_t len = USB_RX_LEN;
  if (len == (sizeof(USB_SETUP_REQ))) {
    SetupLen = ((uint16_t)UsbSetupBuf->wLengthH << 8) | (UsbSetupBuf->wLengthL);
    len = 0;
    SetupReq = UsbSetupBuf->bRequest;
    usbMsgFlags = 0;

    if ((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK) !=
        USB_REQ_TYP_STANDARD) {

      /* ---- Non-standard requests ---- */
      switch ((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK)) {

      case USB_REQ_TYP_VENDOR:
        len = 0xFF;
        break;

      case USB_REQ_TYP_CLASS: {
        __data uint8_t iface = UsbSetupBuf->wIndexL;

        if (iface == 0) {
          /* ---------- Interface 0: Keyboard ---------- */
          switch (SetupReq) {
          case HID_GET_PROTOCOL:
            Ep0Buffer[0] = keyboardProtocol;
            len = 1;
            break;
          case HID_SET_PROTOCOL:
            keyboardProtocol = UsbSetupBuf->wValueL;
            break;
          case HID_SET_IDLE:
            break;
          case HID_SET_REPORT:
            setReportIface = 0;
            setReportType  = UsbSetupBuf->wValueH;
            setReportId    = UsbSetupBuf->wValueL;
            break;
          case HID_GET_REPORT:
            break;
          default:
            len = 0xFF;
            break;
          }

        } else if (iface == 1) {
          /* ---------- Interface 1: Vendor config ---------- */
          switch (SetupReq) {
          case HID_SET_IDLE:
            break;
          case HID_GET_REPORT: {
            __data uint8_t rtype = UsbSetupBuf->wValueH;
            __data uint8_t rid   = UsbSetupBuf->wValueL;
            if (rtype == 3) { /* Feature report */
              /* HID spec: Report ID is first byte in DATA stage */
              if (rid == REPORT_ID_KEYS) {
                Ep0Buffer[0] = REPORT_ID_KEYS;
                config_pack_report2(Ep0Buffer + 1);
                len = 1 + CONFIG_REPORT_SIZE;
              } else if (rid == REPORT_ID_ENCODER) {
                Ep0Buffer[0] = REPORT_ID_ENCODER;
                config_pack_report3(Ep0Buffer + 1);
                len = 1 + CONFIG_REPORT_SIZE;
              } else if (rid == REPORT_ID_LED) {
                Ep0Buffer[0] = REPORT_ID_LED;
                config_pack_report5(Ep0Buffer + 1);
                len = 1 + CONFIG_REPORT_SIZE;
              } else if (rid == REPORT_ID_LONG_KEYS) {
                Ep0Buffer[0] = REPORT_ID_LONG_KEYS;
                config_pack_report6(Ep0Buffer + 1);
                len = 1 + CONFIG_REPORT_SIZE;
              } else if (rid == REPORT_ID_LONG_ENC) {
                Ep0Buffer[0] = REPORT_ID_LONG_ENC;
                config_pack_report7(Ep0Buffer + 1);
                len = 1 + CONFIG_REPORT_SIZE;
              } else {
                len = 0xFF;
              }
            } else {
              len = 0xFF;
            }
            break;
          }
          case HID_SET_REPORT:
            setReportIface = 1;
            setReportType  = UsbSetupBuf->wValueH;
            setReportId    = UsbSetupBuf->wValueL;
            /* data comes in EP0_OUT */
            break;
          case HID_GET_IDLE:
            Ep0Buffer[0] = 0;
            len = 1;
            break;
          default:
            len = 0xFF;
            break;
          }

        } else {
          len = 0xFF;
        }
        break;
      }

      default:
        len = 0xFF;
        break;
      }

    } else {
      /* ---- Standard requests ---- */
      switch (SetupReq) {

      case USB_GET_DESCRIPTOR:
        switch (UsbSetupBuf->wValueH) {
        case 1: /* Device Descriptor */
          pDescr = (__code uint8_t *)DeviceDescriptor;
          len = sizeof(USB_Descriptor_Device_t);
          break;

        case 2: /* Configuration Descriptor */
          pDescr = (__code uint8_t *)ConfigurationDescriptor;
          len = sizeof(USB_Descriptor_Configuration_t);
          break;

        case 3: /* String Descriptors */
          if (UsbSetupBuf->wValueL == 0) {
            pDescr = LanguageDescriptor;
          } else if (UsbSetupBuf->wValueL == 1) {
            pDescr = (__code uint8_t *)ManufacturerDescriptor;
          } else if (UsbSetupBuf->wValueL == 2) {
            pDescr = (__code uint8_t *)ProductDescriptor;
          } else if (UsbSetupBuf->wValueL == 3) {
            pDescr = (__code uint8_t *)SerialDescriptor;
          } else {
            len = 0xff;
            break;
          }
          len = pDescr[0];
          break;

        case 0x22: /* HID Report Descriptor — dispatch by interface (wIndex) */
          if (UsbSetupBuf->wIndexL == 0) {
            pDescr = (__code uint8_t *)KeyboardReportDescriptor;
            len = ConfigurationDescriptor.HID_KeyboardHID.HIDReportLength;
          } else if (UsbSetupBuf->wIndexL == 1) {
            pDescr = (__code uint8_t *)VendorReportDescriptor;
            len = ConfigurationDescriptor.HID_VendorHID.HIDReportLength;
          } else {
            len = 0xff;
          }
          break;

        default:
          len = 0xff;
          break;
        }

        if (len != 0xff) {
          if (SetupLen > len) {
            SetupLen = len;
          }
          len = SetupLen >= DEFAULT_ENDP0_SIZE
                    ? DEFAULT_ENDP0_SIZE
                    : SetupLen;
          for (__data uint8_t i = 0; i < len; i++) {
            Ep0Buffer[i] = pDescr[i];
          }
          SetupLen -= len;
          pDescr += len;
        }
        break;

      case USB_SET_ADDRESS:
        SetupLen = UsbSetupBuf->wValueL;
        break;

      case USB_GET_CONFIGURATION:
        Ep0Buffer[0] = UsbConfig;
        if (SetupLen >= 1) len = 1;
        break;

      case USB_SET_CONFIGURATION:
        UsbConfig = UsbSetupBuf->wValueL;
        break;

      case USB_GET_INTERFACE:
        break;
      case USB_SET_INTERFACE:
        break;

      case USB_CLEAR_FEATURE:
        if ((UsbSetupBuf->bRequestType & 0x1F) == USB_REQ_RECIP_DEVICE) {
          if ((((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01) {
            if (ConfigurationDescriptor.Config.ConfigAttributes & 0x20) {
              /* wake up */
            } else {
              len = 0xFF;
            }
          } else {
            len = 0xFF;
          }
        } else if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) ==
                   USB_REQ_RECIP_ENDP) {
          switch (UsbSetupBuf->wIndexL) {
          case 0x82:
            UEP2_CTRL = UEP2_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
            break;
          case 0x02:
            UEP2_CTRL = UEP2_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
            break;
          case 0x81:
            UEP1_CTRL = UEP1_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
            break;
          case 0x01:
            UEP1_CTRL = UEP1_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
            break;
          default:
            len = 0xFF;
            break;
          }
        } else {
          len = 0xFF;
        }
        break;

      case USB_SET_FEATURE:
        if ((UsbSetupBuf->bRequestType & 0x1F) == USB_REQ_RECIP_DEVICE) {
          if ((((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01) {
            if (!(ConfigurationDescriptor.Config.ConfigAttributes & 0x20)) {
              len = 0xFF;
            }
          } else {
            len = 0xFF;
          }
        } else if ((UsbSetupBuf->bRequestType & 0x1F) == USB_REQ_RECIP_ENDP) {
          if ((((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x00) {
            switch (((uint16_t)UsbSetupBuf->wIndexH << 8) | UsbSetupBuf->wIndexL) {
            case 0x82:
              UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;
              break;
            case 0x02:
              UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;
              break;
            case 0x81:
              UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;
              break;
            case 0x01:
              UEP1_CTRL = UEP1_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;
              break;
            default:
              len = 0xFF;
              break;
            }
          } else {
            len = 0xFF;
          }
        } else {
          len = 0xFF;
        }
        break;

      case USB_GET_STATUS:
        Ep0Buffer[0] = 0x00;
        Ep0Buffer[1] = 0x00;
        if (SetupLen >= 2) len = 2;
        else len = SetupLen;
        break;

      default:
        len = 0xff;
        break;
      }
    }
  } else {
    len = 0xff;
  }

  if (len == 0xff) {
    SetupReq = 0xFF;
    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;
  } else if (len <= DEFAULT_ENDP0_SIZE) {
    UEP0_T_LEN = len;
    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
  } else {
    UEP0_T_LEN = 0;
    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
  }
}

/* ================================================================== */
/*  EP0 IN  (device → host data stage)                                 */
/* ================================================================== */
void USB_EP0_IN() {
  switch (SetupReq) {
  case USB_GET_DESCRIPTOR: {
    __data uint8_t len = SetupLen >= DEFAULT_ENDP0_SIZE
                             ? DEFAULT_ENDP0_SIZE
                             : SetupLen;
    for (__data uint8_t i = 0; i < len; i++) {
      Ep0Buffer[i] = pDescr[i];
    }
    SetupLen -= len;
    pDescr += len;
    UEP0_T_LEN = len;
    UEP0_CTRL ^= bUEP_T_TOG;
  } break;
  case USB_SET_ADDRESS:
    USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    break;
  default:
    UEP0_T_LEN = 0;
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    break;
  }
}

/* ================================================================== */
/*  EP0 OUT (host → device data stage)                                 */
/* ================================================================== */
void USB_EP0_OUT() {
  if (SetupReq == HID_SET_REPORT) {
    if (setReportIface == 1 && setReportType == 3) {
      /* HID spec: Report ID is first byte of DATA stage (Ep0Buffer[0]).
       * Actual report data starts at Ep0Buffer[1]. */
      if (setReportId == REPORT_ID_KEYS) {
        config_unpack_report2(Ep0Buffer + 1);
      } else if (setReportId == REPORT_ID_ENCODER) {
        config_unpack_report3(Ep0Buffer + 1);
      } else if (setReportId == REPORT_ID_LED) {
        config_unpack_report5(Ep0Buffer + 1);
      } else if (setReportId == REPORT_ID_LONG_KEYS) {
        config_unpack_report6(Ep0Buffer + 1);
      } else if (setReportId == REPORT_ID_LONG_ENC) {
        config_unpack_report7(Ep0Buffer + 1);
      } else if (setReportId == REPORT_ID_COMMAND) {
        if (Ep0Buffer[1] == CMD_BOOTLOADER) {
          bootloader_request = 1;  /* defer to main loop so USB STATUS stage completes */
        } else if (Ep0Buffer[1] == CMD_SAVE) {
          config_save();
        }
      }
    } else {
      /* Keyboard LED status (Output report on Interface 0) */
      keyboardLedStatus = Ep0Buffer[0];
    }
  }
  UEP0_T_LEN = 0;
  UEP0_CTRL ^= bUEP_R_TOG;
}

/* ================================================================== */
/*  USB Interrupt Service Routine                                      */
/* ================================================================== */
#pragma save
#pragma nooverlay
void USBInterrupt(void) {
  if (UIF_TRANSFER) {
    __data uint8_t callIndex = USB_INT_ST & MASK_UIS_ENDP;
    switch (USB_INT_ST & MASK_UIS_TOKEN) {
    case UIS_TOKEN_OUT:
      switch (callIndex) {
      case 0: EP0_OUT_Callback();  break;
      case 1: EP1_OUT_Callback();  break;
      case 2: EP2_OUT_Callback();  break;
      case 3: EP3_OUT_Callback();  break;
      case 4: EP4_OUT_Callback();  break;
      default: break;
      }
      break;
    case UIS_TOKEN_SOF:
      switch (callIndex) {
      case 0: EP0_SOF_Callback();  break;
      case 1: EP1_SOF_Callback();  break;
      case 2: EP2_SOF_Callback();  break;
      case 3: EP3_SOF_Callback();  break;
      case 4: EP4_SOF_Callback();  break;
      default: break;
      }
      break;
    case UIS_TOKEN_IN:
      switch (callIndex) {
      case 0: EP0_IN_Callback();   break;
      case 1: EP1_IN_Callback();   break;
      case 2: EP2_IN_Callback();   break;
      case 3: EP3_IN_Callback();   break;
      case 4: EP4_IN_Callback();   break;
      default: break;
      }
      break;
    case UIS_TOKEN_SETUP:
      switch (callIndex) {
      case 0: EP0_SETUP_Callback(); break;
      case 1: EP1_SETUP_Callback(); break;
      case 2: EP2_SETUP_Callback(); break;
      case 3: EP3_SETUP_Callback(); break;
      case 4: EP4_SETUP_Callback(); break;
      default: break;
      }
      break;
    }
    UIF_TRANSFER = 0;
  }

  /* Bus reset */
  if (UIF_BUS_RST) {
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;

    USB_DEV_AD = 0x00;
    UIF_SUSPEND  = 0;
    UIF_TRANSFER = 0;
    UIF_BUS_RST  = 0;

    UsbConfig = 0;
  }

  /* Suspend / wake */
  if (UIF_SUSPEND) {
    UIF_SUSPEND = 0;
    if (!(USB_MIS_ST & bUMS_SUSPEND)) {
      USB_INT_FG = 0xFF;
    }
  }
}
#pragma restore

/* ================================================================== */
/*  USB initialisation helpers                                         */
/* ================================================================== */

void USBDeviceCfg() {
  USB_CTRL = 0x00;
  USB_CTRL &= ~bUC_HOST_MODE;
  USB_CTRL |= bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;
  USB_DEV_AD = 0x00;
  USB_CTRL &= ~bUC_LOW_SPEED;
  UDEV_CTRL &= ~bUD_LOW_SPEED;
#if defined(CH551) || defined(CH552) || defined(CH549)
  UDEV_CTRL = bUD_PD_DIS;
#endif
#if defined(CH559)
  UDEV_CTRL = bUD_DP_PD_DIS;
#endif
  UDEV_CTRL |= bUD_PORT_EN;
}

void USBDeviceIntCfg() {
  USB_INT_EN |= bUIE_SUSPEND;
  USB_INT_EN |= bUIE_TRANSFER;
  USB_INT_EN |= bUIE_BUS_RST;
  USB_INT_FG |= 0x1F;
  IE_USB = 1;
  EA = 1;
}

void USBDeviceEndPointCfg() {
#if defined(CH559)
  UEP0_DMA_H = ((uint16_t)Ep0Buffer >> 8);
  UEP0_DMA_L = ((uint16_t)Ep0Buffer >> 0);
  UEP1_DMA_H = ((uint16_t)Ep1Buffer >> 8);
  UEP1_DMA_L = ((uint16_t)Ep1Buffer >> 0);
  UEP2_DMA_H = ((uint16_t)Ep2Buffer >> 8);
  UEP2_DMA_L = ((uint16_t)Ep2Buffer >> 0);
#else
  UEP0_DMA = (uint16_t)Ep0Buffer;
  UEP1_DMA = (uint16_t)Ep1Buffer;
  UEP2_DMA = (uint16_t)Ep2Buffer;
#endif

  /* EP1 — TX only (keyboard IN) */
  UEP1_CTRL  = bUEP_AUTO_TOG | UEP_T_RES_NAK;
  UEP4_1_MOD = 0x40;       /* bUEP1_TX_EN only */

  /* EP2 — TX only (vendor IN, always NAK) */
  UEP2_CTRL  = bUEP_AUTO_TOG | UEP_T_RES_NAK;
  UEP2_3_MOD = 0x04;       /* bUEP2_TX_EN only */

  /* EP0 — bidirectional control */
  UEP0_CTRL  = UEP_R_RES_ACK | UEP_T_RES_NAK;
}
