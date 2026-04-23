# Screen Sample Code

The following code provides an alternative UI approach, including an arc gauge implementation and different page layouts.

```cpp
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// --- Pins ---
const int BTN_NEXT = 2;
const int BTN_PREV = 3;

// --- State & Data ---
int currentPage = 0;
unsigned long lastDebounceTime = 0;
const int debounceDelay = 250; 

float speed = 0.0;
int batPercent = 80;
int assistLevel = 3; // 1 to 5
int whKm = 15;       // 0 to 40 range

// --- Helper: Draw an Arc Gauge ---
// x, y: center | r: radius | val: current value | maxVal: max possible | label: units
void drawGauge(int x, int y, int r, int val, int maxVal, const char* label) {
  // 1. Draw background "track" (dotted or light)
  u8g2.setDrawColor(1);
  for(int i=0; i<360; i+=10) {
     float rad = i * 0.01745;
     u8g2.drawPixel(x + cos(rad)*r, y + sin(rad)*r);
  }

  // 2. Draw the "Fill" Arc
  // Map value to 0-360 degrees. 
  // We start at 90 degrees (6 o'clock) and wrap around clockwise.
  int endAngle = map(val, 0, maxVal, 0, 359);
  
  // u8g2_draw_arc(x, y, radius, start, end)
  // Note: angles are 0-255 in some u8g2 versions, but usually 0-360 in others.
  // We'll use a loop of lines for maximum compatibility across all U8g2 versions.
  for(int i=0; i <= endAngle; i += 2) {
    float rad = (i + 90) * 0.01745; // Offset by 90 to start at bottom
    u8g2.drawLine(x + cos(rad)*(r-2), y + sin(rad)*(r-2), x + cos(rad)*r, y + sin(rad)*r);
  }

  // 3. Draw the value text in center
  u8g2.setFont(u8g2_font_7x14_tf);
  u8g2.setCursor(x - 4, y + 5);
  u8g2.print(val);
}

void drawSpeed(int x, int y, int fontSize) {
  if (fontSize == 32) u8g2.setFont(u8g2_font_logisoso32_tn);
  else u8g2.setFont(u8g2_font_logisoso20_tn);
  u8g2.setCursor(x, y);
  if(speed < 10) u8g2.print("0");
  u8g2.print((int)speed);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(x + 5, y + 12, "km/h");
}

void drawMainPage() {
  // Battery Bar
  u8g2.drawFrame(2, 5, 12, 54);
  int barHeight = map(batPercent, 0, 100, 0, 50);
  u8g2.drawBox(4, 57 - barHeight, 8, barHeight);
  
  // Power Gauge (1 to 5)
  drawGauge(55, 28, 22, assistLevel, 5, "PWR");
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(40, 62, "ASSIST");

  drawSpeed(90, 40, 32);
}

void drawRangePage() {
  // Wh/km Efficiency Gauge (0 to 40)
  drawGauge(28, 30, 20, whKm, 40, "wh");
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(12, 60, "wh/km");

  // Simple Range Bar
  u8g2.drawFrame(58, 10, 10, 40);
  u8g2.drawBox(60, 20, 6, 28);
  u8g2.drawStr(55, 62, "EST");

  drawSpeed(90, 40, 32);
}

void drawSystemPage() {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(0, 10); u8g2.print("Odo: 1240 km");
  u8g2.setCursor(0, 22); u8g2.print("Trip: 14.2 km");
  u8g2.setCursor(0, 34); u8g2.print("BatT: 24 C");
  u8g2.setCursor(0, 46); u8g2.print("MotT: 38 C");
  u8g2.setCursor(0, 58); u8g2.print("SYS:  OK");
  drawSpeed(95, 45, 20);
}

void setup() {
  u8g2.begin();
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
}

void loop() {
  // Button Handling
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (digitalRead(BTN_NEXT) == LOW) {
      currentPage = (currentPage + 1) % 3;
      lastDebounceTime = millis();
    }
    if (digitalRead(BTN_PREV) == LOW) {
      currentPage = (currentPage - 1 + 3) % 3;
      lastDebounceTime = millis();
    }
  }

  // Data simulation
  speed = 15 + 10 * sin(millis() / 2000.0);
  whKm = 20 + 15 * sin(millis() / 3500.0);

  u8g2.clearBuffer();
  if (currentPage == 0) drawMainPage();
  else if (currentPage == 1) drawRangePage();
  else drawSystemPage();
  u8g2.sendBuffer();
}
```
