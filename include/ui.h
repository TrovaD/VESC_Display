#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "telemetry.h"

// --- Hardware Configuration (FSD 2.0) ---
#define PIN_I2C_SDA  8
#define PIN_I2C_SCL  9

void ui_init();
void ui_update();

#endif
