# Functional Specification Document: VESC_Display (Modular Edition)

## 1. Project Overview
**VESC_Display** is a high-performance telemetry dashboard for ESP32-S3 hardware using native CAN (TWAI). It provides real-time monitoring of VESC motor controllers and BMS units with robust data persistence and modular architecture.

## 2. Hardware Specification
- **MCU:** ESP32-S3.
- **Display:** 0.96" SSD1306 OLED (I2C: SDA GPIO 8, SCL GPIO 9).
- **CAN:** TWAI (TX GPIO 15, RX GPIO 16, 500kbps).
- **Buttons:**
  - **Button 1 (GPIO 6):** Assist Down (Short), Trip Reset (Long).
  - **Button 2 (GPIO 11):** Assist Up (Short), Page Navigation (Long).

## 3. Software Architecture
1. **Telemetry Engine:** Parses VESC Status 1, 2, 3, 4, 5 and BMS Status packets (0x26, 0x29, 0x2B, 0x2D).
2. **Logic Engine:** Handles distance/speed calculations, assist level mapping, and NVS persistence.
3. **UI Engine:** 4-page navigation system with startup animation.

## 4. Functional Requirements

### 4.1 Telemetry Data
- **VESC:** ERPM, Current (Motor/In), Voltage.
- **BMS:** SoC (scaled 0-255), Total Voltage

### 4.2 UI Pages (Enhanced)
1. **DASHBOARD:**
   - **Visuals:** Circular Gauge for power (clockwise)/recuperation (counterclockwise) with separate PAS in the centre (0-4), in the middle a Vertical SoC Bar (no percentage), right side a Large Speed display (on top unit km/h).
2. **POWER METRICS:**
   - **Visuals:** Circular Gauge outside for the for power (clockwise)/recuperation (counterclockwise) and a circular gauge inside for the powermeter with separate Efficiency Gauge (Wh/km), vertical bar with the remaining battery range (maximum km on top), right side a Large Speed display (on top unit km/h).
3. **TRIP COMPUTER:**
   - **Visuals:** Trip/Odo distance, Session Time, Avg Speed, right side a Large Speed display (km/h).
4. **SYSTEM HEALTH:**
   - **Visuals:** left side Odometer and trip and Motor Temperature and SoH, right side a Large Speed display (on top unit km/h).

### 4.3 Support Level Adjustment
- **Levels:** 0 to 3, inside the gauge of the dashboard
- **Communication:** Sends `CAN_PACKET_SET_PAS_SUB_SCALING` (ID 63) to VESC ID 56.
- **Mapping:**
  - Level 0: 0.0 (0%)
  - Level 1: 0.33 (33%)
  - Level 2: 0.66 (66%)
  - Level 3: 1.00 (100%)

### 4.4 Advanced Logic
- **Speed:** Calculated from ERPM integration.
- **Distance:** `delta_dist = (speed_kmh * dt) / 3600000.0f`.
- **Range:** `(SoC / 100 * Capacity_Wh) / Efficiency (EMA)`.
- **Session Reset:** Clears after 30 mins of inactivity.
- **Persistence:** Odo/Trip saved to NVS every 5 mins.

## 5. Technical Constants (Current)
- **Battery Capacity:** 500Wh.
- **VESC ID:** 56 | **BMS ID:** 10.
- **Tire Size:** 2.167m (Wheel Circumference).
- **Gearbox:** 11.3:1 Ratio | **Pole Pairs:** 14.
- **Framework:** Arduino/PlatformIO (ESP32-S3).
