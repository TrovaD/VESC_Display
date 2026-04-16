#include <Arduino.h>
#include "telemetry.h"
#include "logic.h"
#include "ui.h"

void setup() {
    Serial.begin(115200);

    // Initialize Modules
    telemetry_init();
    logic_init();
    ui_init();

    Serial.println("TROVATA Display started");
}

void loop() {
    // Process background tasks
    telemetry_update();
    logic_update();
    ui_update();

    // Yield to avoid watchdog if necessary (though non-blocking)
    yield();
}
