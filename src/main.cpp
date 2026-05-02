#include <Arduino.h>
#include "telemetry.h"
#include "logic.h"
#include "ui.h"
#include "sim_serial.h"

void setup() {
    Serial.begin(115200);

#ifndef SIM_MODE
    telemetry_init();
#endif
    logic_init();
    ui_init();

#ifdef SIM_MODE
    Serial.println("TROVATA Display — SIM MODE");
#else
    Serial.println("TROVATA Display started");
#endif
}

void loop() {
#ifdef SIM_MODE
    sim_serial_update();
#else
    telemetry_update();
#endif
    logic_update();
    ui_update();

    yield();
}
