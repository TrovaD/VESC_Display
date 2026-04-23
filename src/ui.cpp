#include "ui.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

static const uint32_t UI_REFRESH_MS = 16; // UI refresh interval in milliseconds

static uint32_t startup_start_time = 0;
static bool startup_finished = false;

// Scene Prototypes
void scene_dashboard();
void scene_power_metrics();
void scene_trip_computer();
void scene_system_health();
void draw_startup_animation(uint32_t elapsed);

// --- UI Helpers (from Sample) ---

void draw_gauge(int x, int y, int r, int val, int max_val, const char* label) {
    u8g2.setDrawColor(1);
    for(int i=0; i<360; i+=20) {
        float rad = i * 0.01745f;
        u8g2.drawPixel(x + cos(rad)*r, y + sin(rad)*r);
    }

    int end_angle = map(val, 0, max_val, 0, 359);
    for(int i=0; i <= end_angle; i += 2) {
        float rad = (i + 90) * 0.01745f;
        u8g2.drawLine(x + cos(rad)*(r-2), y + sin(rad)*(r-2), x + cos(rad)*r, y + sin(rad)*r);
    }

    u8g2.setFont(u8g2_font_7x14_tf);
    char buf[8];
    sprintf(buf, "%d", val);
    int tw = u8g2.getStrWidth(buf);
    u8g2.drawStr(x - tw/2, y + 5, buf);
    
    u8g2.setFont(u8g2_font_6x10_tf);
    tw = u8g2.getStrWidth(label);
    u8g2.drawStr(x - tw/2, y + r + 10, label);
}

void draw_speed_large(int x, int y, bool large) {
    if (large) u8g2.setFont(u8g2_font_logisoso32_tn);
    else u8g2.setFont(u8g2_font_logisoso20_tn);
    
    char buf[8];
    sprintf(buf, "%02d", (int)state.speed_kmh);
    u8g2.drawStr(x, y, buf);
    
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(x + (large ? 40 : 25), y, "km/h");
}

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
    if (elapsed < 3000 && !startup_finished) { // Reduced startup to 3s
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
    int x_offset = (int)(elapsed * 0.04);

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(x_offset + 16, 32, "__o"); 
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
    // Battery Vertical Bar
    u8g2.drawFrame(2, 5, 12, 54);
    int bar_height = map((int)state.bms_soc, 0, 100, 0, 50);
    u8g2.drawBox(4, 57 - bar_height, 8, bar_height);
    
    // Assist Level Gauge (0-4 mapped to gauge)
    draw_gauge(55, 28, 20, state.assist_level, 4, "PAS");

    // Speed
    draw_speed_large(85, 40, true);

    // Bottom Stats
    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[16];
    sprintf(buf, "%.0fW", state.power);
    u8g2.drawStr(20, 62, buf);

    sprintf(buf, "%.1fkm", state.remaining_range);
    int tw = u8g2.getStrWidth(buf);
    u8g2.drawStr(124 - tw, 62, buf);
}

void scene_power_metrics() {
    // Efficiency Gauge
    draw_gauge(30, 30, 18, (int)state.efficiency, 50, "Wh/km");

    // Metrics List
    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];
    sprintf(buf, "V: %.1fV", state.bms_voltage);
    u8g2.drawStr(65, 15, buf);
    
    float calc_amps = state.motor_current * state.duty_cycle;
    sprintf(buf, "A: %.1fA", calc_amps);
    u8g2.drawStr(65, 28, buf);

    sprintf(buf, "P: %.0fW", state.power);
    u8g2.drawStr(65, 41, buf);

    draw_speed_large(75, 60, false);
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
    
    draw_speed_large(95, 45, false);
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

    draw_speed_large(95, 45, false);
}

