#include "ui.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

static const uint32_t UI_REFRESH_MS = 20; // UI refresh interval in milliseconds

static uint32_t startup_start_time = 0;
static bool startup_finished = false;

// Scene Prototypes
void scene_dashboard();
void scene_power_metrics();
void scene_trip_computer();
void scene_system_health();
void draw_startup_animation(uint32_t elapsed);

// --- UI Helpers ---

void draw_circular_gauge(int x, int y, int r, float val, float max_val, float min_val, bool bidirectional) {
    u8g2.setDrawColor(1);
    // Background track (dotted)
    for(int i=0; i<360; i+=20) {
        float rad = i * 0.01745f;
        u8g2.drawPixel(x + cos(rad)*r, y + sin(rad)*r);
    }

    // 0 is at 270 degrees (Top)
    if (bidirectional) {
        if (val >= 0) {
            int angle = map((int)constrain(val, 0, max_val), 0, (int)max_val, 0, 180);
            for(int i=0; i<=angle; i+=2) {
                float rad = (270 + i) * 0.01745f;
                u8g2.drawLine(x + cos(rad)*(r-2), y + sin(rad)*(r-2), x + cos(rad)*r, y + sin(rad)*r);
            }
        } else {
            int angle = map((int)constrain(fabsf(val), 0, fabsf(min_val)), 0, (int)fabsf(min_val), 0, 180);
            for(int i=0; i<=angle; i+=2) {
                float rad = (270 - i) * 0.01745f;
                u8g2.drawLine(x + cos(rad)*(r-2), y + sin(rad)*(r-2), x + cos(rad)*r, y + sin(rad)*r);
            }
        }
    } else {
        int angle = map((int)constrain(val, 0, max_val), 0, (int)max_val, 0, 359);
        for(int i=0; i<=angle; i+=2) {
            float rad = (270 + i) * 0.01745f;
            u8g2.drawLine(x + cos(rad)*(r-2), y + sin(rad)*(r-2), x + cos(rad)*r, y + sin(rad)*r);
        }
    }
}

void draw_speed_large(int x, int y, bool large) {
    if (large) u8g2.setFont(u8g2_font_logisoso32_tn);
    else u8g2.setFont(u8g2_font_logisoso20_tn);
    
    char buf[8];
    sprintf(buf, "%02d", (int)state.speed_kmh);
    u8g2.drawStr(x, y, buf);
    
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(x + (large ? 42 : 28), y, "km/h");
}

void draw_secondary_speed() {
    char buf[16];
    u8g2.setFont(u8g2_font_6x10_tf);
    sprintf(buf, "%d km/h", (int)state.speed_kmh);
    int tw = u8g2.getStrWidth(buf);
    u8g2.drawStr(127 - tw, 10, buf);
}

void ui_init() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    u8g2.begin();
    startup_start_time = millis();
}

void ui_update() {
    static uint32_t last_draw = 0;
    if (millis() - last_draw < UI_REFRESH_MS) return;
    last_draw = millis();

    u8g2.clearBuffer();
    u8g2.setDrawColor(1);

    uint32_t elapsed = millis() - startup_start_time;
    if (elapsed < 3000 && !startup_finished) {
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

    if (frame % 2 == 0) u8g2.drawStr(x_offset, 51, "(x)/ (x)");
    else u8g2.drawStr(x_offset, 51, "(+)/ (+)");

    int road_shift = (frame % 10);
    for(int i = -10; i < 128; i += 10) {
        u8g2.drawStr(i - road_shift, 60, "~~~~~");
    }

    u8g2.setFont(u8g2_font_lubBI10_tf);
    u8g2.drawStr(30, 20, "TROVATA");
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(105, 10, "V2.0");
}

void scene_dashboard() {
    // Left: Power Gauge
    draw_circular_gauge(30, 30, 26, state.power, 2000, -500, true);
    
    // PAS in center of gauge
    u8g2.setFont(u8g2_font_logisoso20_tn);
    char pas_buf[4];
    sprintf(pas_buf, "%d", state.assist_level);
    int tw = u8g2.getStrWidth(pas_buf);
    u8g2.drawStr(30 - tw/2, 38, pas_buf);
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(24, 15, "PAS");

    // Middle: SoC Bar
    u8g2.drawFrame(62, 10, 8, 44);
    int bar_height = map((int)state.bms_soc, 0, 100, 0, 40);
    u8g2.drawBox(64, 52 - bar_height, 4, bar_height);
    u8g2.drawStr(60, 8, "BAT");

    // Right: Large Speed
    draw_speed_large(82, 42, true);

    // Bottom Stats
    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];
    sprintf(buf, "%.0fW", state.power);
    u8g2.drawStr(5, 62, buf);

    sprintf(buf, "%.1fkm", state.remaining_range);
    tw = u8g2.getStrWidth(buf);
    u8g2.drawStr(127 - tw, 62, buf);
}

void scene_power_metrics() {
    // Left: Concentric Gauges
    // Outer: Power/Recup
    draw_circular_gauge(30, 30, 26, state.power, 2000, -500, true);
    // Inner: Efficiency
    draw_circular_gauge(30, 30, 18, state.efficiency, 50, 0, false);
    
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(18, 32, "Wh/km");

    // Metrics List
    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];
    sprintf(buf, "V:%.1fV", state.bms_voltage > 0 ? state.bms_voltage : state.input_voltage);
    u8g2.drawStr(65, 15, buf);
    sprintf(buf, "A:%.1fA", state.input_current);
    u8g2.drawStr(65, 27, buf);
    sprintf(buf, "P:%.0fW", state.power);
    u8g2.drawStr(65, 39, buf);
    sprintf(buf, "U:%.2fAh", state.amp_hours);
    u8g2.drawStr(65, 51, buf);

    // Range Bar (Vertical)
    u8g2.drawFrame(120, 15, 6, 35);
    int r_height = map((int)constrain(state.remaining_range, 0, 100), 0, 100, 0, 31);
    u8g2.drawBox(122, 48 - r_height, 2, r_height);
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(115, 12, "km");

    draw_secondary_speed();
}

void scene_trip_computer() {
    char buf[32];
    u8g2.setFont(u8g2_font_6x10_tf);
    
    sprintf(buf, "T:%.2fkm", state.trip_distance);
    u8g2.drawStr(0, 15, buf);
    sprintf(buf, "O:%.1fkm", state.odometer);
    u8g2.drawStr(0, 27, buf);

    uint32_t active_s = (millis() - state.session_start_time) / 1000;
    sprintf(buf, "%02d:%02d:%02d", active_s/3600, (active_s%3600)/60, active_s%60);
    u8g2.drawStr(0, 39, buf);

    sprintf(buf, "Avg:%.1fk", state.avg_speed);
    u8g2.drawStr(0, 51, buf);
    
    draw_speed_large(82, 42, true);
    
    u8g2.drawLine(0, 54, 128, 54);
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 62, "TRIP COMPUTER");
}

void scene_system_health() {
    char buf[32];
    u8g2.setFont(u8g2_font_6x10_tf);
    
    sprintf(buf, "Mot:%.1fC", state.motor_temp);
    u8g2.drawStr(0, 15, buf);
    sprintf(buf, "FET:%.1fC", state.fet_temp);
    u8g2.drawStr(0, 27, buf);
    sprintf(buf, "SoH:%.1f%%", state.bms_soh);
    u8g2.drawStr(0, 39, buf);
    sprintf(buf, "CAN:%.0f/s", state.can_load);
    u8g2.drawStr(0, 51, buf);

    draw_speed_large(82, 42, true);

    u8g2.drawLine(0, 54, 128, 54);
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 62, "SYSTEM HEALTH");
}


