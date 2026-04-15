# Functional Specification Document: VESC_Display_2 (Modular Edition)

## 1. Project Overview
**VESC_Display_2** is a high-performance telemetry dashboard inspired by the OpenSourceEBike Modular DIY project. It is optimized for ESP32-S3 hardware using native CAN (TWAI) to interface directly with VESC motor controllers and BMS units. The project prioritizes a modular software architecture for easy UI customization and robust data persistence.

## 2. Hardware Specification
- **MCU:** ESP32-S3 (Dual-core, native USB/CAN).
- **Display:** 0.96" SSD1306 OLED (128x64, I2C).
- **Communication:**
  - **CAN Bus:** 500kbps (standard VESC bitrate).
  - **I2C:** Display (SDA: GPIO 8, SCL: GPIO 9).
- **Inputs:**
  - **Button 1 (GPIO 6):** support level decrease (Short Press),Page Navigation (long press)
  - **Button 2 (GPIO 11):** support level increase (short press), Trip Reset (Long Press).

## 3. Software Architecture (Modular Design)
The firmware is divided into three logical layers to mirror the modularity of the reference repository:
1. **Telemetry Engine (CAN):** Asynchronous task that decodes VESC Status messages into a global `SystemState` struct.
2. **Logic Engine:** Calculates derived values (Power, Efficiency, Wh/km, SoC from Voltage).
3. **UI Engine (U8g2):** A page-manager that renders "Scenes" based on the current state.

## 4. Functional Requirements

### 4.1 Telemetry Data (VESC CAN Status)
| Source | Data Points |
| :--- | :--- |
| **VESC Status 1** | ERPM, Current (Motor), Duty Cycle. |
| **VESC Status 2** | Amp Hours, Amp Hours Charged. |
| **VESC Status 4** | Temp (FET), Temp (Motor), Current (In). |
| **VESC Status 5** | Voltage (In), Tachometer. |
| **BMS Status** | SoC (%), Cell Voltages, Hottest Cell. |

### 4.2 UI Pages (Scenes)
1. **DASHBOARD (Main):**
   - Large Speedometer (km/h).
   - Battery Bar (Visual) + SoC %.
   - Real-time Power (Watts).
2. **POWER METRICS:**
   - Current (Amps) + Voltage.
   - Efficiency (Wh/km).
   - Capacity used (Ah).
3. **TRIP COMPUTER:**
   - Trip Distance + Odometer.
   - Travel Time (Session).
   - Average Speed.
4. **SYSTEM HEALTH (Diagnostics):**
   - MOSFET & Motor Temps.
   - VESC Fault Codes (Flags).
   - CAN Bus Load/Status.

### 4.3 Advanced Logic
- **Power Calculation:** `Watts = InputVoltage * InputCurrent`.
- **Distance Calculation:** `delta_dist = (delta_tacho / (POLE_PAIRS * 6.0 * GEAR_RATIO)) * WHEEL_CIRCUMFERENCE`.
- **Speed Calculation:** `km/h = (delta_dist_km * 3,600,000) / delta_time_ms`.
- **Automatic Trip Management:**
  - Save Odometer/Trip to NVS every 5 minutes.
  - Reset "Session" distance after 30 minutes of 0 ERPM.
- **Dynamic Refresh:** UI updates at 10Hz (100ms), CAN polling is opportunistic (loop-speed).

## 5. Technical Constraints
- **Framework:** Arduino/PlatformIO (C++).
- **Libraries:** `U8g2` (Display), `Preferences` (NVS), `driver/twai.h` (CAN).
- **Memory:** Use internal NVS for Odometer persistence to survive power cycles.
