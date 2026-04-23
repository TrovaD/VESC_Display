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
                case 0x0E: { // CAN_PACKET_STATUS_2: Amp Hours
                    uint32_t u_ah = ((uint32_t)msg.data[0] << 24 | (uint32_t)msg.data[1] << 16 | (uint32_t)msg.data[2] << 8 | (uint32_t)msg.data[3]);
                    memcpy(&state.amp_hours, &u_ah, 4);
                    break;
                }
                case 0x10: { // CAN_PACKET_STATUS_4: Temp FET, Temp Motor, Current In
                    int16_t fet    = (int16_t)((uint16_t)msg.data[0] << 8 | (uint16_t)msg.data[1]);
                    int16_t mot    = (int16_t)((uint16_t)msg.data[2] << 8 | (uint16_t)msg.data[3]);
                    int16_t cur_in = (int16_t)((uint16_t)msg.data[4] << 8 | (uint16_t)msg.data[5]);
                    state.fet_temp = (float)fet / 10.0f;
                    state.motor_temp = (float)mot / 10.0f;
                    state.input_current = (float)cur_in / 10.0f;
                    break;
                }
                case 0x1B: { // CAN_PACKET_STATUS_5: Tachometer, Voltage
                    int32_t tacho = (int32_t)((uint32_t)msg.data[0] << 24 | (uint32_t)msg.data[1] << 16 | (uint32_t)msg.data[2] << 8 | (uint32_t)msg.data[3]);
                    int16_t volt  = (int16_t)((uint16_t)msg.data[4] << 8 | (uint16_t)msg.data[5]);
                    state.tachometer = (float)tacho;
                    state.input_voltage = (float)volt / 10.0f;
                    break;
                }
            }
        } else if (node == BMS_ID) {
            switch(cmd) {
                case 38: { // CAN_PACKET_BMS_V_TOT (0x26): float32
                    uint32_t u_v = ((uint32_t)msg.data[0] << 24 | (uint32_t)msg.data[1] << 16 | (uint32_t)msg.data[2] << 8 | (uint32_t)msg.data[3]);
                    memcpy(&state.bms_voltage, &u_v, 4);
                    break;
                }
                case 41: { // CAN_PACKET_BMS_I (0x29): float32
                    uint32_t u_i = ((uint32_t)msg.data[0] << 24 | (uint32_t)msg.data[1] << 16 | (uint32_t)msg.data[2] << 8 | (uint32_t)msg.data[3]);
                    float bms_i;
                    memcpy(&bms_i, &u_i, 4);
                    // If we have BMS current, it's a better source for input_current
                    state.input_current = bms_i; 
                    break;
                }
                case 43: { // CAN_PACKET_BMS_TEMPS (0x2B): float16 scaled by 100.0
                    uint8_t start_idx = msg.data[0];
                    if (start_idx == 0) {
                        int16_t t1 = (int16_t)((uint16_t)msg.data[1] << 8 | (uint16_t)msg.data[2]);
                        state.bms_hottest_cell = (float)t1 / 100.0f;
                    }
                    break;
                }
                case 45: { // CAN_PACKET_BMS_SOC_SOH_TEMP_STAT (0x2D)
                    state.bms_soc = (float)msg.data[4] / 2.55f; // 0-255 -> 0.0-100.0%
                    state.bms_soh = (float)msg.data[5] / 2.55f; // 0-255 -> 0.0-100.0%
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

void telemetry_send_assist(float rel_current) {
    twai_message_t msg;
    msg.identifier = (10 << 8) | VESC_ID; // CAN_PACKET_SET_CURRENT_REL = 10
    msg.extd = 1;
    msg.data_length_code = 4;
    
    uint32_t u_val;
    memcpy(&u_val, &rel_current, 4);
    
    msg.data[0] = (u_val >> 24) & 0xFF;
    msg.data[1] = (u_val >> 16) & 0xFF;
    msg.data[2] = (u_val >> 8) & 0xFF;
    msg.data[3] = u_val & 0xFF;

    twai_transmit(&msg, pdMS_TO_TICKS(10));
}
