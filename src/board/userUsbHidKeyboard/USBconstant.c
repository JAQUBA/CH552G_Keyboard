/*
 * USBconstant.c — USB descriptors for composite HID device
 *   Interface 0: HID Keyboard    (EP1 IN — keystrokes)
 *   Interface 1: HID Vendor      (EP2 IN — dummy; Feature Reports via EP0)
 */

#include "USBconstant.h"

/* ================================================================== */
/*  Report Descriptors (defined BEFORE ConfigurationDescriptor so      */
/*  that sizeof() is available for HIDReportLength fields)             */
/* ================================================================== */

/* --- Keyboard Report Descriptor (keyboard + consumer control) ---- */
__code uint8_t KeyboardReportDescriptor[] = {
    /* Report ID 1: Standard Keyboard */
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x06, // USAGE (Keyboard)
    0xa1, 0x01, // COLLECTION (Application)
    0x85, 0x01, //   REPORT_ID (1)
    0x05, 0x07, //   USAGE_PAGE (Keyboard)
    0x19, 0xe0, //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7, //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x25, 0x01, //   LOGICAL_MAXIMUM (1)
    0x95, 0x08, //   REPORT_COUNT (8)
    0x75, 0x01, //   REPORT_SIZE (1)
    0x81, 0x02, //   INPUT (Data,Var,Abs)
    0x95, 0x01, //   REPORT_COUNT (1)
    0x75, 0x08, //   REPORT_SIZE (8)
    0x81, 0x03, //   INPUT (Cnst,Var,Abs)
    0x95, 0x06, //   REPORT_COUNT (6)
    0x75, 0x08, //   REPORT_SIZE (8)
    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x25, 0xff, //   LOGICAL_MAXIMUM (255)
    0x05, 0x07, //   USAGE_PAGE (Keyboard)
    0x19, 0x00, //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0xe7, //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x81, 0x00, //   INPUT (Data,Ary,Abs)
    0xc0,       // END_COLLECTION

    /* Report ID 2: Consumer Control (multimedia keys) */
    0x05, 0x0C, // USAGE_PAGE (Consumer)
    0x09, 0x01, // USAGE (Consumer Control)
    0xA1, 0x01, // COLLECTION (Application)
    0x85, 0x02, //   REPORT_ID (2)
    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x03, //   LOGICAL_MAXIMUM (1023)
    0x19, 0x00, //   USAGE_MINIMUM (0)
    0x2A, 0xFF, 0x03, //   USAGE_MAXIMUM (1023)
    0x75, 0x10, //   REPORT_SIZE (16)
    0x95, 0x01, //   REPORT_COUNT (1)
    0x81, 0x00, //   INPUT (Data,Ary,Abs)
    0xC0        // END_COLLECTION
};

/* --- Vendor Report Descriptor (config Feature Reports) ------------ */
__code uint8_t VendorReportDescriptor[] = {
    0x06, 0x00, 0xFF, // USAGE_PAGE (Vendor Defined 0xFF00)
    0x09, 0x01,       // USAGE (Vendor Usage 1)
    0xA1, 0x01,       // COLLECTION (Application)

    /* Dummy 1-byte Input report (required for IN endpoint) */
    0x85, 0x01,       //   REPORT_ID (1)
    0x09, 0x01,       //   USAGE (Vendor Usage 1)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00, //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x95, 0x01,       //   REPORT_COUNT (1)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)

    /* Feature Report ID 2 — key mapping (7 bytes) */
    0x85, 0x02,       //   REPORT_ID (2)
    0x09, 0x02,       //   USAGE (Vendor Usage 2)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00, //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x95, 0x07,       //   REPORT_COUNT (7)
    0xB1, 0x02,       //   FEATURE (Data,Var,Abs)

    /* Feature Report ID 3 — encoder mapping (7 bytes) */
    0x85, 0x03,       //   REPORT_ID (3)
    0x09, 0x03,       //   USAGE (Vendor Usage 3)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00, //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x95, 0x07,       //   REPORT_COUNT (7)
    0xB1, 0x02,       //   FEATURE (Data,Var,Abs)

    /* Feature Report ID 4 — command (1 byte) */
    0x85, 0x04,       //   REPORT_ID (4)
    0x09, 0x04,       //   USAGE (Vendor Usage 4)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00, //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x95, 0x01,       //   REPORT_COUNT (1)
    0xB1, 0x02,       //   FEATURE (Data,Var,Abs)

    /* Feature Report ID 5 — LED configuration (7 bytes) */
    0x85, 0x05,       //   REPORT_ID (5)
    0x09, 0x05,       //   USAGE (Vendor Usage 5)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00, //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x95, 0x07,       //   REPORT_COUNT (7)
    0xB1, 0x02,       //   FEATURE (Data,Var,Abs)

    /* Feature Report ID 6 — long-press key mapping (7 bytes) */
    0x85, 0x06,       //   REPORT_ID (6)
    0x09, 0x06,       //   USAGE (Vendor Usage 6)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00, //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x95, 0x07,       //   REPORT_COUNT (7)
    0xB1, 0x02,       //   FEATURE (Data,Var,Abs)

    /* Feature Report ID 7 — long-press encoder extended (7 bytes) */
    0x85, 0x07,       //   REPORT_ID (7)
    0x09, 0x07,       //   USAGE (Vendor Usage 7)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00, //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x95, 0x07,       //   REPORT_COUNT (7)
    0xB1, 0x02,       //   FEATURE (Data,Var,Abs)

    0xC0              // END_COLLECTION
};

/* ================================================================== */
/*  Device Descriptor                                                  */
/* ================================================================== */

__code USB_Descriptor_Device_t DeviceDescriptor = {
    .Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

    .USBSpecification = VERSION_BCD(1, 1, 0),
    .Class = 0x00,
    .SubClass = 0x00,
    .Protocol = 0x00,

    .Endpoint0Size = DEFAULT_ENDP0_SIZE,

    .VendorID  = 0x1209,
    .ProductID = 0xC55D,
    .ReleaseNumber = VERSION_BCD(1, 1, 0),

    .ManufacturerStrIndex = 1,
    .ProductStrIndex = 2,
    .SerialNumStrIndex = 3,

    .NumberOfConfigurations = 1
};

/* ================================================================== */
/*  Configuration Descriptor  (2 interfaces)                           */
/* ================================================================== */

__code USB_Descriptor_Configuration_t ConfigurationDescriptor = {
    .Config = {
        .Header = {
            .Size = sizeof(USB_Descriptor_Configuration_Header_t),
            .Type = DTYPE_Configuration
        },
        .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
        .TotalInterfaces = 2,
        .ConfigurationNumber = 1,
        .ConfigurationStrIndex = NO_DESCRIPTOR,
        .ConfigAttributes = (USB_CONFIG_ATTR_RESERVED),
        .MaxPowerConsumption = USB_CONFIG_POWER_MA(200)
    },

    /* ---- Interface 0: HID Keyboard ---- */
    .HID_KeyboardInterface = {
        .Header = {.Size = sizeof(USB_Descriptor_Interface_t),
                   .Type = DTYPE_Interface},
        .InterfaceNumber = 0,
        .AlternateSetting = 0x00,
        .TotalEndpoints = 1,
        .Class    = HID_CSCP_HIDClass,
        .SubClass = 0x00,           /* non-boot (Report IDs used) */
        .Protocol = 0x00,           /* none */
        .InterfaceStrIndex = NO_DESCRIPTOR
    },

    .HID_KeyboardHID = {
        .Header = {.Size = sizeof(USB_HID_Descriptor_HID_t),
                   .Type = HID_DTYPE_HID},
        .HIDSpec = VERSION_BCD(1, 1, 0),
        .CountryCode = 0x00,
        .TotalReportDescriptors = 1,
        .HIDReportType = HID_DTYPE_Report,
        .HIDReportLength = sizeof(KeyboardReportDescriptor)
    },

    .HID_KeyboardINEndpoint = {
        .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t),
                   .Type = DTYPE_Endpoint},
        .EndpointAddress = KEYBOARD_EPADDR,
        .Attributes = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC |
                       ENDPOINT_USAGE_DATA),
        .EndpointSize = KEYBOARD_EPSIZE,
        .PollingIntervalMS = 10
    },

    /* ---- Interface 1: HID Vendor (config) ---- */
    .HID_VendorInterface = {
        .Header = {.Size = sizeof(USB_Descriptor_Interface_t),
                   .Type = DTYPE_Interface},
        .InterfaceNumber = 1,
        .AlternateSetting = 0x00,
        .TotalEndpoints = 1,
        .Class    = HID_CSCP_HIDClass,
        .SubClass = 0x00,           /* non-boot */
        .Protocol = 0x00,           /* none     */
        .InterfaceStrIndex = NO_DESCRIPTOR
    },

    .HID_VendorHID = {
        .Header = {.Size = sizeof(USB_HID_Descriptor_HID_t),
                   .Type = HID_DTYPE_HID},
        .HIDSpec = VERSION_BCD(1, 1, 0),
        .CountryCode = 0x00,
        .TotalReportDescriptors = 1,
        .HIDReportType = HID_DTYPE_Report,
        .HIDReportLength = sizeof(VendorReportDescriptor)
    },

    .HID_VendorINEndpoint = {
        .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t),
                   .Type = DTYPE_Endpoint},
        .EndpointAddress = VENDOR_EPADDR,
        .Attributes = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC |
                       ENDPOINT_USAGE_DATA),
        .EndpointSize = VENDOR_EPSIZE,
        .PollingIntervalMS = 100
    }
};

/* ================================================================== */
/*  String Descriptors                                                 */
/* ================================================================== */

__code uint8_t LanguageDescriptor[] = {
    0x04, 0x03, 0x09, 0x04
};

__code uint16_t SerialDescriptor[] = {
    (((9 + 1) * 2) | (DTYPE_String << 8)),
    'C', 'H', '5', '5', 'x', ' ', 'k', 'b', 'd'
};

__code uint16_t ProductDescriptor[] = {
    (((15 + 1) * 2) | (DTYPE_String << 8)),
    'C', 'H', '5', '5', '2', 'G', ' ', 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd'
};

__code uint16_t ManufacturerDescriptor[] = {
    (((6 + 1) * 2) | (DTYPE_String << 8)),
    'J', 'A', 'Q', 'U', 'B', 'A'
};
