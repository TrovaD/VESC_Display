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
    pinMode(PIN_BUTTON_1, INPUT_PULLUP);
    pinMode(PIN_BUTTON_2, INPUT_PULLUP);

    prefs.begin("vesc_display_2", false);
    state.odometer = prefs.getFloat("odometer", 0.0);
    state.trip_distance = prefs.getFloat("trip", 0.0);
    state.assist_level = prefs.getUChar("assist", 0);
    
    last_save_time = millis();
    last_activity_time = millis();
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
    }
}

// B1 Long: Page Nav
void b1_long() {
    state.current_page = (state.current_page + 1) % 4;
}

// B2 Short: Increase Assist
void b2_short() {
    if (state.assist_level < 9) {
        state.assist_level++;
        prefs.putUChar("assist", state.assist_level);
    }
}

// B2 Long: Reset Trip
void b2_long() {
    state.trip_distance = 0;
    prefs.putFloat("trip", 0.0);
}

void logic_update() {
    handle_button(btn1, b1_short, b1_long);
    handle_button(btn2, b2_short, b2_long);

    // Distance Calculation (FSD 4.3 with *6.0 fix)
    if (state.tachometer != 0) {
        if (prev_tacho != 0) {
            float delta_tacho = state.tachometer - prev_tacho;
            // 1 mechanical rev = pole_pairs * 6 steps
            float delta_dist = (delta_tacho / (POLE_PAIRS * 6.0 * GEAR_RATIO)) * WHEEL_CIRCUMFERENCE;
            static uint32_t last_tacho_time = 0;
            uint32_t now = millis();
            
            if (delta_dist > 0) {
                float dist_km = delta_dist / 1000.0;
                state.odometer += dist_km;
                state.trip_distance += dist_km;
                state.travel_distance += dist_km;
                last_activity_time = now;

                if (last_tacho_time != 0) {
                    uint32_t dt = now - last_tacho_time;
                    if (dt > 0) {
                        state.speed_kmh = (dist_km * 3600000.0) / dt;
                    }
                }
            } else {
                state.speed_kmh = 0;
            }
            last_tacho_time = now;
        }
        prev_tacho = state.tachometer;
    } else {
        state.speed_kmh = 0;
    }

    // Odometer Persistence (every 5 mins)
    if (millis() - last_save_time > 300000) {
        prefs.putFloat("odometer", state.odometer);
        prefs.putFloat("trip", state.trip_distance);
        last_save_time = millis();
    }

    // Inactivity Reset (30 mins)
    if (millis() - last_activity_time > 1800000) {
        state.travel_distance = 0;
        // Don't reset last_activity_time here, it will be reset when tacho moves
    }
}
