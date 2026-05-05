# Communication Protocol - VESC Display

The display functions as a telemetry monitor and user interface for the eBike system.

## CAN-Bus (TWAI)
The display connects to the shared CAN-Bus to receive data from the VESC and BMS, and to send user commands.

### Received Data (from VESC)
The display parses VESC status packets to show:
- ERPM / Speed
- Motor & Input Current
- Duty Cycle
- FET & Motor Temperatures
- Battery Voltage (from VESC side)
- Amp Hours consumed

### Received Data (from BMS)
The display parses BMS packets to show:
- State of Charge (SOC%)
- State of Health (SOH%)
- Hottest cell temperature
- Individual cell voltages

### Sent Commands
- **Assist Level:** Sends `CAN_PACKET_SET_PAS_SUB_SCALING` (Command 63) to the VESC to adjust the assist intensity based on the user's selected mode.
- Format: Big-endian int32 scaled by 100,000.

## Hardware
- **Interface:** ESP32 TWAI (Two-Wire Automotive Interface).
- **Transceiver:** Required (e.g., SN65HVD230).
