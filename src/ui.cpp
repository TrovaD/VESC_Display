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
    u8g2.setFont(u8g2_font_6x10_tf);
    int lw = u8g2.getStrWidth("km/h");
    int nw = large ? 42 : 28;
    u8g2.drawStr(x + (nw - lw) / 2, y - (large ? 34 : 24), "km/h");

    if (large) u8g2.setFont(u8g2_font_logisoso32_tn);
    else u8g2.setFont(u8g2_font_logisoso20_tn);

    char buf[8];
    sprintf(buf, "%02d", (int)state.speed_kmh);
    u8g2.drawStr(x, y, buf);
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
    // Column 1: Power & PAS (0-42)
    draw_circular_gauge(21, 30, 20, state.power, 2000, -500, true);
    
    // Inverted PAS Label
    u8g2.setDrawColor(1);
    u8g2.drawBox(6, 0, 30, 11);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(12, 9, "PAS");
    u8g2.setDrawColor(1);

    // PAS Value in center of gauge
    u8g2.setFont(u8g2_font_logisoso20_tn);
    char pas_buf[4];
    sprintf(pas_buf, "%d", state.assist_level);
    int tw = u8g2.getStrWidth(pas_buf);
    u8g2.drawStr(21 - tw/2, 40, pas_buf);

    // Column 2: Battery (44-57)
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(44, 8, "BAT");
    u8g2.drawFrame(45, 10, 10, 44);
    int bar_height = map((int)constrain(state.bms_soc, 0, 100), 0, 100, 0, 40);
    u8g2.drawBox(47, 52 - bar_height, 6, bar_height);
    
    // Column 3: Speed (59-128)
    draw_speed_large(62, 42, true);
}

void scene_power_metrics() {
    // Left: Concentric Gauges
    // Outer: Power/Recup
    draw_circular_gauge(30, 30, 26, state.power, 2000, -500, true);
    // Inner: Efficiency
    draw_circular_gauge(30, 30, 18, state.efficiency, 50, 0, false);
    
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(18, 32, "Wh/km");

    // Range Bar (Vertical)
    u8g2.drawFrame(120, 15, 6, 35);
    int r_height = map((int)constrain(state.remaining_range, 0, 100), 0, 100, 0, 31);
    u8g2.drawBox(122, 48 - r_height, 2, r_height);
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(115, 12, "km");

    draw_speed_large(62, 44, false);
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
    
    sprintf(buf, "Odo:%.1fkm", state.odometer);
    u8g2.drawStr(0, 15, buf);
    sprintf(buf, "Trp:%.2fkm", state.trip_distance);
    u8g2.drawStr(0, 27, buf);
    sprintf(buf, "Mot:%.1fC", state.motor_temp);
    u8g2.drawStr(0, 39, buf);
    sprintf(buf, "SoH:%.1f%%", state.bms_soh);
    u8g2.drawStr(0, 51, buf);

    draw_speed_large(82, 42, true);

    u8g2.drawLine(0, 54, 128, 54);
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 62, "SYSTEM HEALTH");
}


