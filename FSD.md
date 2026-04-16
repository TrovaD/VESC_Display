# Functional Specification Document: VESC_Display_2 (Modular Edition)

## 1. Project Overview
**VESC_Display_2** is a high-performance telemetry dashboard for ESP32-S3 hardware using native CAN (TWAI). It provides real-time monitoring of VESC motor controllers and BMS units with robust data persistence and modular architecture.

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
- **VESC:** ERPM, Current (Motor/In), Duty, Ah, Tachometer, Voltage.
- **BMS:** SoC (scaled 0-255), Total Voltage, Cell Voltages (16 cells), Hottest Cell Temp.

### 4.2 UI Pages
1. **DASHBOARD:** Speed, SoC Bar, PAS Level, Power (W), Range (km).
2. **POWER METRICS:** Voltage, Current, Efficiency (Wh/km), Amp Hours used.
3. **TRIP COMPUTER:** Trip/Odo distance, Session Time, Avg Speed.
4. **SYSTEM HEALTH:** FET/Motor/BMS Temps, CAN load (msg/s).

### 4.3 Support Level Adjustment (New)
- **Levels:** 0 to 4.
- **Communication:** Sends `CAN_PACKET_SET_CURRENT_REL` (ID 10) to VESC.
- **Mapping:** 
  - Level 0: 0.0 (0%)
  - Level 1: 0.25 (25%)
  - Level 2: 0.50 (50%)
  - Level 3: 0.75 (75%)
  - Level 4: 1.0 (100%)

### 4.4 Advanced Logic
- **Speed:** Calculated from Tachometer with ERPM fallback.
- **Distance:** `delta_dist = fabsf(delta_tacho / (PP * 6 * GR)) * Circumference`.
- **Range:** `(SoC / 100 * Capacity_Wh) / Efficiency (EMA)`.
- **Session Reset:** Clears after 30 mins of inactivity.
- **Persistence:** Odo/Trip saved to NVS every 5 mins.

## 5. Technical Constraints
- **Battery Capacity:** 500Wh (default).
- **VESC ID:** 56, **BMS ID:** 10.
- **Framework:** Arduino/PlatformIO.
