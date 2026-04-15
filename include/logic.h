#ifndef LOGIC_H
#define LOGIC_H

#include <Arduino.h>
#include <Preferences.h>
#include "telemetry.h"

// --- Hardware Configuration (FSD 2.0) ---
#define PIN_BUTTON_1 6
#define PIN_BUTTON_2 11

// --- Motor/Wheel Config (FSD 4.3) ---
#define POLE_PAIRS 7.0
#define GEAR_RATIO 1.0
#define WHEEL_CIRCUMFERENCE 2.0 // Meters

void logic_init();
void logic_update();

#endif
