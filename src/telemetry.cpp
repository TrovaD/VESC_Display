#include "telemetry.h"

SystemState state;

void telemetry_init() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)PIN_CAN_TX, (gpio_num_t)PIN_CAN_RX, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = CAN_SPEED;
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        twai_start();
    }
    state.start_time = millis();
}

static uint32_t msg_count = 0;
static uint32_t last_load_calc = 0;

void telemetry_update() {
    twai_message_t msg;
    while (twai_receive(&msg, 0) == ESP_OK) {
        msg_count++;
        if (!msg.extd) continue;

        uint32_t id = msg.identifier;
        uint32_t cmd = (id >> 8); // Command is bits 8-28
        uint8_t node = id & 0xFF; // Node ID is bits 0-7

        if (node == VESC_ID) {
            state.last_can_activity = millis();
            switch(cmd) {
                case 0x09: { // CAN_PACKET_STATUS (Status 1): ERPM, Current, Duty
                    int32_t erpm = (int32_t)((uint32_t)msg.data[0] << 24 | (uint32_t)msg.data[1] << 16 | (uint32_t)msg.data[2] << 8 | (uint32_t)msg.data[3]);
                    int16_t cur  = (int16_t)((uint16_t)msg.data[4] << 8 | (uint16_t)msg.data[5]);
                    int16_t duty = (int16_t)((uint16_t)msg.data[6] << 8 | (uint16_t)msg.data[7]);
                    state.erpm = (float)erpm;
                    state.motor_current = (float)cur / 10.0f;
                    state.duty_cycle = (float)duty / 1000.0f;
                    break;
                }
                case 0x0E: { // CAN_PACKET_STATUS_2 (Status 2): Amp Hours, Amp Hours Charged
                    int32_t ah = (int32_t)((uint32_t)msg.data[0] << 24 | (uint32_t)msg.data[1] << 16 | (uint32_t)msg.data[2] << 8 | (uint32_t)msg.data[3]);
                    state.amp_hours = (float)ah / 10000.0f;
                    break;
                }
                case 0x0F: { // CAN_PACKET_STATUS_3 (Status 3): Watt Hours, Watt Hours Charged
                    // FSD doesn't have a field for this yet, but we could add it to SystemState if needed
                    break;
                }
                case 0x10: { // CAN_PACKET_STATUS_4 (Status 4): Temp FET, Temp Motor, Current In
                    int16_t fet    = (int16_t)((uint16_t)msg.data[0] << 8 | (uint16_t)msg.data[1]);
                    int16_t mot    = (int16_t)((uint16_t)msg.data[2] << 8 | (uint16_t)msg.data[3]);
                    int16_t cur_in = (int16_t)((uint16_t)msg.data[4] << 8 | (uint16_t)msg.data[5]);
                    state.fet_temp = (float)fet / 10.0f;
                    state.motor_temp = (float)mot / 10.0f;
                    state.input_current = (float)cur_in / 10.0f;
                    break;
                }
                case 0x1B: { // CAN_PACKET_STATUS_5 (Status 5): Tachometer, Voltage In
                    int32_t tacho = (int32_t)((uint32_t)msg.data[0] << 24 | (uint32_t)msg.data[1] << 16 | (uint32_t)msg.data[2] << 8 | (uint32_t)msg.data[3]);
                    int16_t volt  = (int16_t)((uint16_t)msg.data[4] << 8 | (uint16_t)msg.data[5]);
                    state.tachometer = (float)tacho;
                    state.input_voltage = (float)volt / 10.0f;
                    break;
                }
            }
        } else if (node == BMS_ID) {
            switch(cmd) {
                case 38: { // CAN_PACKET_BMS_V_TOT (0x26)
                    int32_t v_tot = (int32_t)((uint32_t)msg.data[0] << 24 | (uint32_t)msg.data[1] << 16 | (uint32_t)msg.data[2] << 8 | (uint32_t)msg.data[3]);
                    state.bms_voltage = (float)v_tot / 1000.0f;
                    break;
                }
                case 43: { // CAN_PACKET_BMS_TEMPS (0x2B)
                    uint8_t start_idx = msg.data[0];
                    if (start_idx == 0) { // We only take the first hottest if we want bms_hottest_cell
                        int16_t t1 = (int16_t)((uint16_t)msg.data[1] << 8 | (uint16_t)msg.data[2]);
                        state.bms_hottest_cell = (float)t1 / 100.0f;
                    }
                    break;
                }
                case 41: { // CAN_PACKET_BMS_V_CELL (0x29)
                    uint8_t start_idx = msg.data[0];
                    for (int i = 0; i < 3; i++) {
                        if (start_idx + i < 16) {
                            uint16_t v = (uint16_t)((uint16_t)msg.data[1 + i*2] << 8 | (uint16_t)msg.data[2 + i*2]);
                            state.cell_voltages[start_idx + i] = (float)v / 1000.0f;
                        }
                    }
                    break;
                }
                case 45: { // CAN_PACKET_BMS_SOC_SOH_TEMP_STAT (0x2D)
                    state.bms_soc = (float)msg.data[4] / 2.55f; // 0-255 -> 0-100%
                    
                    // Fallback for hottest cell
                    if (state.bms_hottest_cell <= 0) {
                        state.bms_hottest_cell = (float)((int8_t)msg.data[6]);
                    }
                    break;
                }
            }
        }
    }

    if (millis() - last_load_calc > 1000) {
        state.can_load = msg_count;
        msg_count = 0;
        last_load_calc = millis();
    }
}
