#ifdef SIM_MODE
#include "sim_serial.h"
#include "telemetry.h"

static String sim_buf = "";

void sim_serial_update() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n') {
            int eq = sim_buf.indexOf('=');
            if (eq > 0) {
                String key = sim_buf.substring(0, eq);
                float val  = sim_buf.substring(eq + 1).toFloat();

                if      (key == "speed")     state.speed_kmh        = val;
                else if (key == "soc")       state.bms_soc          = val;
                else if (key == "power")     state.power            = val;
                else if (key == "range")     state.remaining_range  = val;
                else if (key == "voltage")   state.input_voltage    = val;
                else if (key == "bms_volt")  state.bms_voltage      = val;
                else if (key == "current")   state.input_current    = val;
                else if (key == "mot_cur")   state.motor_current    = val;
                else if (key == "duty")      state.duty_cycle       = val;
                else if (key == "amp_hours") state.amp_hours        = val;
                else if (key == "effic")     state.efficiency       = val;
                else if (key == "trip")      state.trip_distance    = val;
                else if (key == "odometer")  state.odometer         = val;
                else if (key == "avg_speed") state.avg_speed        = val;
                else if (key == "mot_temp")  state.motor_temp       = val;
                else if (key == "fet_temp")  state.fet_temp         = val;
                else if (key == "bms_soh")   state.bms_soh          = val;
                else if (key == "can_load")  state.can_load         = val;
                else if (key == "bms_temp")  state.bms_hottest_cell = val;
                else if (key == "assist")    state.assist_level     = (uint8_t)val;
                else if (key == "page")      state.current_page     = (uint8_t)val;
            }
            sim_buf = "";
        } else if (sim_buf.length() < 32) {
            sim_buf += c;
        }
    }
}
#endif
