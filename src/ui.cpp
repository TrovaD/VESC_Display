#include "ui.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

static const uint32_t UI_REFRESH_MS = 100; // UI refresh interval in milliseconds

static uint32_t startup_start_time = 0;
static bool startup_finished = false;

// Scene Prototypes
void scene_dashboard();
void scene_power_metrics();
void scene_trip_computer();
void scene_system_health();
void draw_startup_animation(uint32_t elapsed);

void ui_init() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    u8g2.begin();
    startup_start_time = millis();
}

void ui_update() {
    static uint32_t last_draw = 0;
    if (millis() - last_draw < UI_REFRESH_MS) return; // UI refresh
    last_draw = millis();

    u8g2.clearBuffer();

    uint32_t elapsed = millis() - startup_start_time;
    if (elapsed < 5000 && !startup_finished) {
        draw_startup_animation(elapsed);
    } else {
        startup_finished = true;
        switch(state.current_page) {
            case 0: scene_dashboard(); break;
            case 1: scene_power_metrics(); break;
            case 2: scene_trip_computer(); break;
            case 3: scene_system_health(); break;
            default: state.current_page = 0; break;
        }
    }

    u8g2.sendBuffer();
}

void draw_startup_animation(uint32_t elapsed) {
    int frame = elapsed / UI_REFRESH_MS;
    int x_offset = (int)(elapsed * 0.03);

    u8g2.setFont(u8g2_font_6x10_tf);
    // ASCII Bike refined
    //    __o
    //  _`\<,_
    // (x)/ (x)
    u8g2.drawStr(x_offset + 16, 32, "__o"); // Neck fix: moved up 1px
    u8g2.drawStr(x_offset + 4, 43, "_`\\<,_");

    if (frame % 2 == 0) {
        u8g2.drawStr(x_offset, 51, "(x)/ (x)");
    } else {
        u8g2.drawStr(x_offset, 51, "(+)/ (+)");
    }

    int road_shift = (frame % 10);
    for(int i = -10; i < 128; i += 10) {
        u8g2.drawStr(i - road_shift, 60, "~~~~~");
    }

    u8g2.setFont(u8g2_font_lubBI10_tf);
    u8g2.drawStr(30, 20, "TROVATA");
}

void scene_dashboard() {
    char buf[16];
    u8g2.drawFrame(0, 0, 128, 10);
    int bar_width = (int)((state.bms_soc / 100.0) * 124);
    if (bar_width > 0) u8g2.drawBox(2, 2, bar_width, 6);

    // Assist Level
    u8g2.setFont(u8g2_font_6x10_tf);
    sprintf(buf, "PAS %d", state.assist_level);
    u8g2.setDrawColor(0); // Invert
    u8g2.drawStr(55, 8, buf);
    u8g2.setDrawColor(1);

    u8g2.setFont(u8g2_font_logisoso24_tr);
    sprintf(buf, "%.1f", state.speed_kmh);
    u8g2.drawStr(5, 45, buf);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(80, 45, "km/h");

    sprintf(buf, "%.0fW", state.power);
    u8g2.drawStr(5, 62, buf);

    sprintf(buf, "%.1fkm", state.remaining_range);
    u8g2.drawStr(45, 62, buf);

    sprintf(buf, "%.0f%%", state.bms_soc);
    u8g2.drawStr(95, 62, buf);
}

void scene_power_metrics() {
    char buf[32];
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "POWER METRICS");
    u8g2.drawLine(0, 12, 128, 12);

    sprintf(buf, "Voltage: %.1f V", state.bms_voltage); // Use BMS voltage
    u8g2.drawStr(0, 25, buf);
    
    float calc_amps = state.motor_current * state.duty_cycle;
    sprintf(buf, "Current: %.1f A", calc_amps);
    u8g2.drawStr(0, 38, buf);

    sprintf(buf, "Effic:   %.1f Wh/km", state.efficiency);
    u8g2.drawStr(0, 51, buf);

    // Ah is no longer available in telemetry
    u8g2.drawStr(0, 64, "Used:    --- Ah");
}

void scene_trip_computer() {
    char buf[32];
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "TRIP COMPUTER");
    u8g2.drawLine(0, 12, 128, 12);

    sprintf(buf, "Trip: %.2f km", state.trip_distance);
    u8g2.drawStr(0, 25, buf);
    sprintf(buf, "Odo:  %.1f km", state.odometer);
    u8g2.drawStr(0, 38, buf);

    uint32_t active_s = (millis() - state.session_start_time) / 1000;
    sprintf(buf, "Time: %02d:%02d:%02d", active_s/3600, (active_s%3600)/60, active_s%60);
    u8g2.drawStr(0, 51, buf);

    sprintf(buf, "Avg:  %.1f km/h", state.avg_speed);
    u8g2.drawStr(0, 64, buf);
}

void scene_system_health() {
    char buf[32];
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "SYSTEM HEALTH");
    u8g2.drawLine(0, 12, 128, 12);

    sprintf(buf, "FET:   %.1f C", state.fet_temp);
    u8g2.drawStr(0, 25, buf);
    sprintf(buf, "Motor: %.1f C", state.motor_temp);
    u8g2.drawStr(0, 38, buf);
    
    sprintf(buf, "BMS:   %.1f C", state.bms_hottest_cell);
    u8g2.drawStr(0, 51, buf);

    sprintf(buf, "CAN:   %.0f msg/s", state.can_load);
    u8g2.drawStr(0, 64, buf);
}
