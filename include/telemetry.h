#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>
#include "driver/twai.h"

// --- Hardware Configuration (FSD 2.0) ---
#define PIN_CAN_TX   15 
#define PIN_CAN_RX   16

// --- VESC/BMS Configuration (FSD 3.3) ---
#define VESC_ID      56 
#define BMS_ID       10
#define CAN_SPEED    TWAI_TIMING_CONFIG_500KBITS()

struct SystemState {
    float fet_temp = 0;
    float motor_temp = 0;
    float motor_current = 0;
    float input_current = 0;
    float duty_cycle = 0;
    float input_voltage = 0;
    float bms_voltage = 0;
    float tachometer = 0;
    float erpm = 0;
    float amp_hours = 0;
    float bms_soc = 0;
    float bms_soh = 0;
    float bms_hottest_cell = 0;
    float cell_voltages[16] = {0};
    float speed_kmh = 0;
    float odometer = 0;
    float trip_distance = 0;
    float travel_distance = 0;
    uint32_t start_time = 0;
    uint32_t fault_code = 0;
    uint32_t last_can_activity = 0;
    float avg_speed = 0;
    float power = 0;
    float efficiency = 0;
    float remaining_range = 0;
    float can_load = 0; // % of expected messages
    
    // Additional Logic State
    uint8_t assist_level = 0;
    uint8_t current_page = 0;
    uint32_t session_start_time = 0;
};

extern SystemState state;

void telemetry_init();
void telemetry_update();
void telemetry_send_assist(float rel_current);

#endif
