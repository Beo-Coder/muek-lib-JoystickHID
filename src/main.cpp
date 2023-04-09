// Copyright (c) 2023. Leonhard Baschang

#include <Arduino.h>
#include "JoystickHID.h"

JoystickHID joystick(6, 8, true);

void setup() {

}

void loop() {
    joystick.setButton(0, 1);
    joystick.setButton(3, 1);
    joystick.setButton(4, 1);

    for (int i = 0; i < 1028; i++) {
        joystick.setAxis(0, i);
        joystick.setAxis(1, -i);
    }

    delay(100);
}