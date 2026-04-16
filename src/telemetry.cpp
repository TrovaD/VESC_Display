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
        uint8_t cmd = (id >> 8) & 0xFF;
        uint8_t node = id & 0xFF;

        if (node == VESC_ID) {
            state.last_can_activity = millis();
            switch(cmd) {
                case 0x09: { // Status 1: ERPM, Current, Duty
                    int16_t cur = (msg.data[4] << 8) | msg.data[5];
                    int16_t duty = (msg.data[6] << 8) | msg.data[7];
                    state.motor_current = cur / 10.0;
                    state.duty_cycle = duty / 1000.0;
                    break;
                }
                case 0x0E: { // Status 2: Amp Hours
                    int32_t ah = (msg.data[0] << 24) | (msg.data[1] << 16) | (msg.data[2] << 8) | msg.data[3];
                    state.amp_hours = ah / 10000.0;
                    break;
                }
                case 0x10: { // Status 4: Temps & Input Current
                    int16_t fet = (msg.data[0] << 8) | msg.data[1];
                    int16_t mot = (msg.data[2] << 8) | msg.data[3];
                    int16_t cur_in = (msg.data[4] << 8) | msg.data[5];
                    state.fet_temp = fet / 10.0;
                    state.motor_temp = mot / 10.0;
                    state.input_current = cur_in / 10.0;
                    break;
                }
                case 0x1B: { // Status 5: Tachometer & Voltage
                    int32_t tacho = (msg.data[0] << 24) | (msg.data[1] << 16) | (msg.data[2] << 8) | msg.data[3];
                    uint16_t volt = (msg.data[4] << 8) | msg.data[5];
                    state.tachometer = (float)tacho;
                    state.input_voltage = volt / 10.0;
                    break;
                }
                case 0x1C: { // Status 6: Fault Code
                    state.fault_code = msg.data[3];
                    break;
                }
            }
        } else if (node == BMS_ID) {
            if (cmd == 0x2D) { // BMS SOC/Temp
                state.bms_soc = (msg.data[4] / 255.0) * 100.0;
                state.bms_hottest_cell = (float)msg.data[6];
            }
        }
    }

    if (millis() - last_load_calc > 1000) {
        state.can_load = msg_count; // Messages per second
        msg_count = 0;
        last_load_calc = millis();
    }
}
