#ifndef LOGIC_H
#define LOGIC_H

#include <Arduino.h>
#include <Preferences.h>
#include "telemetry.h"

// --- Hardware Configuration (FSD 2.0) ---
#define PIN_BUTTON_1 6
#define PIN_BUTTON_2 11

// --- Motor/Wheel Config (FSD 4.3) ---
#define POLE_PAIRS 14.0
#define GEAR_RATIO 11.3
#define WHEEL_CIRCUMFERENCE 2.167 // Meters

// --- Battery Config ---
#define BATTERY_CAPACITY_WH 500.0

void logic_init();
void logic_update();

#endif
