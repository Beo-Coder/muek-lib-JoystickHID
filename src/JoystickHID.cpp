// Copyright (c) 2023. Leonhard Baschang


#include "PluggableUSBHID.h"
#include "JoystickHID.h"

#define JOYSTICK_AXIS_MINIMUM 0
#define JOYSTICK_AXIS_MAXIMUM 65535

using namespace arduino;

JoystickHID::JoystickHID(uint8_t axisCount, uint8_t buttonCount, bool autoSend) {
    this->autoSend = autoSend;

    if (axisCount > MAX_AXIS_COUNT) {
        this->axisCount = MAX_AXIS_COUNT;
    } else {
        this->axisCount = axisCount;
    }



    //Declare array for axis
    if (this->axisCount > 0) {
        axisState = new int32_t[this->axisCount];
        axisMinimum = new int32_t[this->axisCount];
        axisMaximum = new int32_t[this->axisCount];



        //Init values for axes
        for (int i = 0; i < this->axisCount; i++) {
            axisState[i] = 0;
            axisMinimum[i] = -1028;
            axisMaximum[i] = 1028;
        }
    }

    if (buttonCount > MAX_BUTTON_COUNT) {
        this->buttonCount = MAX_BUTTON_COUNT;
    } else if (buttonCount < 1) {
        this->buttonCount = 1;
    } else {
        this->buttonCount = buttonCount;
    }


    //Declare array for buttons
    if (this->buttonCount > 0) {
        buttonValuesArraySize = this->buttonCount / 8;
        if ((this->buttonCount % 8) > 0) {
            buttonValuesArraySize++;
        }
        buttonValues = new uint8_t[buttonValuesArraySize];

        //Init values for buttons
        for (int i = 0; i < this->buttonCount; i++) {
            buttonValues[i] = 0;

        }
    }

    sendState();


}


void JoystickHID::setAxis(uint8_t axisIndex, int32_t value) {
    if (axisIndex < axisCount) {
        axisState[axisIndex] = value;
    }
    if (autoSend) sendState();

}

void JoystickHID::setButton(uint8_t buttonIndex, uint8_t value) {
    if (value == 0) {
        releaseButton(buttonIndex);
    } else {
        pressButton(buttonIndex);
    }

}

void JoystickHID::pressButton(uint8_t buttonIndex) {
    if (buttonIndex >= buttonCount) return;

    int index = buttonIndex / 8;
    int bit = buttonIndex % 8;

    bitSet(buttonValues[index], bit);
    if (autoSend) sendState();

}

void JoystickHID::releaseButton(uint8_t buttonIndex) {
    if (buttonIndex >= buttonCount) return;

    int index = buttonIndex / 8;
    int bit = buttonIndex % 8;

    bitClear(buttonValues[index], bit);
    if (autoSend) sendState();

}

int JoystickHID::buildAndSet16BitValue(int32_t value, int32_t valueMinimum, int32_t valueMaximum, int32_t actualMinimum,
                                       int32_t actualMaximum, uint8_t dataLocation[]) {
    int32_t convertedValue;
    uint8_t highByte;
    uint8_t lowByte;
    int32_t realMinimum = min(valueMinimum, valueMaximum);
    int32_t realMaximum = max(valueMinimum, valueMaximum);

    if (value < realMinimum) {
        value = realMinimum;
    }
    if (value > realMaximum) {
        value = realMaximum;
    }

    if (valueMinimum > valueMaximum) {
        // Values go from a larger number to a smaller number (e.g. 1024 to 0)
        value = realMaximum - value + realMinimum;
    }

    convertedValue = map(value, realMinimum, realMaximum, actualMinimum, actualMaximum);

    highByte = (uint8_t) (convertedValue >> 8);
    lowByte = (uint8_t) (convertedValue & 0x00FF);

    dataLocation[0] = lowByte;
    dataLocation[1] = highByte;

    return 2;
}

int
JoystickHID::buildAndSetAxisValue(int32_t axisValue, int32_t axisMinimum, int32_t axisMaximum, uint8_t dataLocation[]) {
    return buildAndSet16BitValue(axisValue, axisMinimum, axisMaximum, JOYSTICK_AXIS_MINIMUM,
                                 JOYSTICK_AXIS_MAXIMUM, dataLocation);
}

bool JoystickHID::sendState() {
    _mutex.lock();

    HID_REPORT report;
    report.data[0] = DEFAULT_REPORT_ID;
    uint8_t index = 1;

    // Load Button State
    for (; index < buttonValuesArraySize + 1; index++) {
        report.data[index] = buttonValues[index - 1];
    }
    // Set Axis Value
    for (int i = 0; i < axisCount; i++) {
        index += buildAndSetAxisValue(axisState[i], axisMinimum[i], axisMaximum[i], &(report.data[index]));
    }


    report.length = index;

    if (!send(&report)) {
        _mutex.unlock();
        return false;
    }

    _mutex.unlock();
    return true;
}


const uint8_t *JoystickHID::report_desc() {
    uint8_t buttonsInLastByte = buttonCount % 8;
    uint8_t buttonPaddingBits = 0;
    if (buttonsInLastByte > 0) {
        buttonPaddingBits = 8 - buttonsInLastByte;
    }


    static uint8_t reportDescriptor[] = {
            0x05, 0x01, // USAGE_PAGE (Generic Desktop)
            0x09, DEFAULT_DEVICE_ID, // USAGE (Joystick)
            0xa1, 0x01, // COLLECTION (Application)
            0x85, DEFAULT_REPORT_ID, //   REPORT_ID (3)

            0x05, 0x09, // USAGE_PAGE (Button)
            0x19, 0x01, // USAGE_MINIMUM (Button 1)
            0x29, 0x80, // USAGE_MAXIMUM (Button 128)
            0x15, 0x00, // LOGICAL_MINIMUM (0)
            0x25, 0x01, // LOGICAL_MAXIMUM (1)
            0x95, 0x80, // REPORT_COUNT (128)
            0x75, 0x01, // REPORT_SIZE (1)
            0x81, 0x02, // INPUT (Data,Var,Abs)


            0x75, 0x01, // REPORT_SIZE (1)
            0x95, 0x00, // REPORT_COUNT (# of padding bits)
            0x81, 0x03, // INPUT (Const,Var,Abs)



            0x05, 0x01, // USAGE_PAGE (Generic Desktop) // analog axes
            0x09, 0x30, // USAGE (X)
            0x09, 0x31, // USAGE (Y)
            0x09, 0x32, // USAGE (Z)
            0x09, 0x33, // USAGE (Rx)
            0x09, 0x34, // USAGE (Ry)
            0x09, 0x35, // USAGE (Rz)
            0x15, 0x00, //LOGICAL_MINIMUM (0)
            0x27, 0xFF, 0xFF, 0x00, 0x00, //LOGICAL_MAXIMUM (65535)
            0x75, 0x10, // REPORT_SIZE (16)
            0x95, 0x06, // REPORT_COUNT (6)
            0x81, 0x02, // INPUT (Data, Variable, Absolute)

            0xc0 // END_COLLECTION

    };

    //I know this is scuffed
    reportDescriptor[13] = buttonCount;
    reportDescriptor[19] = buttonCount;
    reportDescriptor[27] = buttonPaddingBits;
    reportDescriptor[54] = axisCount;


    reportLength = sizeof(reportDescriptor);
    return reportDescriptor;
}


#define DEFAULT_CONFIGURATION (1)
#define TOTAL_DESCRIPTOR_LENGTH ((1 * CONFIGURATION_DESCRIPTOR_LENGTH) + (1 * INTERFACE_DESCRIPTOR_LENGTH) + (1 * HID_DESCRIPTOR_LENGTH) + (2 * ENDPOINT_DESCRIPTOR_LENGTH))

const uint8_t *JoystickHID::configuration_desc(uint8_t index) {
    if (index != 0) {
        return nullptr;
    }
    uint8_t configuration_descriptor_temp[] = {
            CONFIGURATION_DESCRIPTOR_LENGTH, // bLength
            CONFIGURATION_DESCRIPTOR,        // bDescriptorType
            LSB(TOTAL_DESCRIPTOR_LENGTH),    // wTotalLength (LSB)
            MSB(TOTAL_DESCRIPTOR_LENGTH),    // wTotalLength (MSB)
            0x01,                            // bNumInterfaces
            DEFAULT_CONFIGURATION,           // bConfigurationValue
            0x00,                            // iConfiguration
            C_RESERVED | C_SELF_POWERED,     // bmAttributes
            C_POWER(0),                      // bMaxPower

            INTERFACE_DESCRIPTOR_LENGTH, // bLength
            INTERFACE_DESCRIPTOR,        // bDescriptorType
            0x00,                        // bInterfaceNumber
            0x00,                        // bAlternateSetting
            0x02,                        // bNumEndpoints
            HID_CLASS,                   // bInterfaceClass
            HID_SUBCLASS_BOOT,           // bInterfaceSubClass
            HID_PROTOCOL_KEYBOARD,       // bInterfaceProtocol
            0x00,                        // iInterface

            HID_DESCRIPTOR_LENGTH,                // bLength
            HID_DESCRIPTOR,                       // bDescriptorType
            LSB(HID_VERSION_1_11),                // bcdHID (LSB)
            MSB(HID_VERSION_1_11),                // bcdHID (MSB)
            0x00,                                 // bCountryCode
            0x01,                                 // bNumDescriptors
            REPORT_DESCRIPTOR,                    // bDescriptorType
            (uint8_t) (LSB(report_desc_length())), // wDescriptorLength (LSB)
            (uint8_t) (MSB(report_desc_length())), // wDescriptorLength (MSB)

            ENDPOINT_DESCRIPTOR_LENGTH, // bLength
            ENDPOINT_DESCRIPTOR,        // bDescriptorType
            _int_in,                    // bEndpointAddress
            E_INTERRUPT,                // bmAttributes
            LSB(MAX_HID_REPORT_SIZE),   // wMaxPacketSize (LSB)
            MSB(MAX_HID_REPORT_SIZE),   // wMaxPacketSize (MSB)
            1,                          // bInterval (milliseconds)

            ENDPOINT_DESCRIPTOR_LENGTH, // bLength
            ENDPOINT_DESCRIPTOR,        // bDescriptorType
            _int_out,                   // bEndpointAddress
            E_INTERRUPT,                // bmAttributes
            LSB(MAX_HID_REPORT_SIZE),   // wMaxPacketSize (LSB)
            MSB(MAX_HID_REPORT_SIZE),   // wMaxPacketSize (MSB)
            1,                          // bInterval (milliseconds)
    };
    MBED_ASSERT(sizeof(configuration_descriptor_temp) == sizeof(_configuration_descriptor));
    memcpy(_configuration_descriptor, configuration_descriptor_temp, sizeof(_configuration_descriptor));
    return _configuration_descriptor;
}
