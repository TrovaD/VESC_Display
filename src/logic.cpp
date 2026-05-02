#include "logic.h"

Preferences prefs;
static float prev_tacho = 0;
static uint32_t last_save_time = 0;
static uint32_t last_activity_time = 0;

// Button State
struct Button {
    uint8_t pin;
    bool last_state;
    uint32_t press_start;
    bool long_pressed;

    Button(uint8_t p) : pin(p), last_state(HIGH), press_start(0), long_pressed(false) {}
};

static Button btn1(PIN_BUTTON_1);
static Button btn2(PIN_BUTTON_2);

void logic_init() {
#ifndef SIM_MODE
    pinMode(PIN_BUTTON_1, INPUT_PULLUP);
    pinMode(PIN_BUTTON_2, INPUT_PULLUP);
#endif

    prefs.begin("vesc_display_2", false);
    state.odometer = prefs.getFloat("odometer", 0.0);
    state.trip_distance = prefs.getFloat("trip", 0.0);
    state.assist_level = prefs.getUChar("assist", 0);
    
    last_save_time = millis();
    last_activity_time = millis();
    state.session_start_time = millis();
}

void handle_button(Button &b, void (*short_press)(), void (*long_press)()) {
    bool current_state = digitalRead(b.pin);
    
    if (b.last_state == HIGH && current_state == LOW) { // Pressed
        b.press_start = millis();
        b.long_pressed = false;
    } 
    else if (b.last_state == LOW && current_state == HIGH) { // Released
        if (!b.long_pressed) {
            short_press();
        }
    }
    
    if (current_state == LOW && !b.long_pressed) {
        if (millis() - b.press_start > 1000) { // 1 second threshold
            long_press();
            b.long_pressed = true;
        }
    }
    
    b.last_state = current_state;
}

// B1 Short: Decrease Assist
void b1_short() {
    if (state.assist_level > 0) {
        state.assist_level--;
        prefs.putUChar("assist", state.assist_level);
        telemetry_send_assist(state.assist_level * (1.0f / 3.0f));
    }
}

// B1 Long: Reset Trip
void b1_long() {
    state.trip_distance = 0;
    prefs.putFloat("trip", 0.0);
}

// B2 Short: Increase Assist
void b2_short() {
    if (state.assist_level < 3) {
        state.assist_level++;
        prefs.putUChar("assist", state.assist_level);
        telemetry_send_assist(state.assist_level * (1.0f / 3.0f));
    }
}

// B2 Long: Page Nav
void b2_long() {
    state.current_page = (state.current_page + 1) % 4;
}

static uint32_t moving_time_ms = 0;
static uint32_t last_logic_time = 0;

void logic_update() {
    uint32_t now = millis();
    uint32_t dt_logic = (last_logic_time == 0) ? 0 : now - last_logic_time;
    last_logic_time = now;

#ifndef SIM_MODE
    handle_button(btn1, b1_short, b1_long);
    handle_button(btn2, b2_short, b2_long);
#endif

#ifndef SIM_MODE
    // Speed (km/h) = (ERPM / Pole_Pairs / Gear_Ratio) * Circumference * 60 / 1000
    state.speed_kmh = fabsf((state.erpm / (POLE_PAIRS * GEAR_RATIO)) * WHEEL_CIRCUMFERENCE * 60.0f / 1000.0f);

    // Distance Calculation (ERPM Integration)
    if (state.speed_kmh > 0.5f) {
        float dist_km = (state.speed_kmh * dt_logic) / 3600000.0f;
        state.odometer += dist_km;
        state.trip_distance += dist_km;
        state.travel_distance += dist_km;
        last_activity_time = now;
        moving_time_ms += dt_logic;
    }

    // Average Speed Calculation
    if (moving_time_ms > 0) {
        state.avg_speed = (state.travel_distance * 3600000.0f) / moving_time_ms;
    }

    // Power Calculation (Using best available Voltage and Current)
    float active_volt = (state.bms_voltage > 0) ? state.bms_voltage : state.input_voltage;
    state.power = active_volt * state.input_current;

    // Efficiency Calculation (Wh/km)
    if (state.speed_kmh > 1.0f) {
        float instant_eff = state.power / state.speed_kmh;
        if (state.efficiency <= 0) state.efficiency = instant_eff;
        else state.efficiency = state.efficiency * 0.95f + instant_eff * 0.05f;
    }

    if (state.efficiency > 0) {
        state.remaining_range = (state.bms_soc / 100.0f * BATTERY_CAPACITY_WH) / state.efficiency;
    } else {
        state.remaining_range = 0;
    }
#endif

    // Odometer Persistence (every 5 mins)
    if (millis() - last_save_time > 300000) {
        prefs.putFloat("odometer", state.odometer);
        prefs.putFloat("trip", state.trip_distance);
        last_save_time = millis();
    }

    // Inactivity Reset (30 mins)
    if (millis() - last_activity_time > 1800000) {
        state.travel_distance = 0;
        state.session_start_time = millis();
        moving_time_ms = 0;
        last_activity_time = millis();
    }
}
