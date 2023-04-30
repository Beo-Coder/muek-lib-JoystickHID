// Copyright (c) 2023. Leonhard Baschang



#ifndef JOYSTICKHID_H
#define JOYSTICKHID_H

#define HID_MAX_AXIS_COUNT 9
#define HID_MAX_BUTTON_COUNT 128

#define DEFAULT_REPORT_ID 0x03
#define DEFAULT_DEVICE_ID 0x04


#include "PluggableUSBHID.h"
#include "platform/Stream.h"
#include "PlatformMutex.h"

namespace arduino {
    class JoystickHID : public USBHID {
        uint8_t axisCount;
        uint8_t buttonCount;

        uint8_t buttonValuesArraySize = 0;
        uint8_t *buttonValues = nullptr;

        int32_t *axisState = nullptr;
        int32_t *axisMinimum = nullptr;
        int32_t *axisMaximum = nullptr;

        bool autoSend = true;

    public:
        JoystickHID(uint8_t axisCount, uint8_t buttonCount, bool autoSend = true);

        bool sendState();

        void setAxis(uint8_t axisIndex, int32_t value);

        void setButton(uint8_t buttonIndex, uint8_t value);

        void pressButton(uint8_t buttonIndex);

        void releaseButton(uint8_t buttonIndex);

    protected:
        const uint8_t *configuration_desc(uint8_t index) override;

    private:
        uint8_t _configuration_descriptor[41];
        PlatformMutex _mutex;

        const uint8_t *report_desc() override;

        int buildAndSetAxisValue(int32_t axisValue, int32_t axisMinimum, int32_t axisMaximum, uint8_t dataLocation[]);

        int buildAndSet16BitValue(int32_t value, int32_t valueMinimum, int32_t valueMaximum, int32_t actualMinimum,
                                  int32_t actualMaximum, uint8_t dataLocation[]);


    };
};


#endif //JOYSTICKHID_H
