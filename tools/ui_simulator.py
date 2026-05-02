#!/usr/bin/env python3
"""
TROVATA Display UI Simulator
Streams state values over USB-serial to ESP32 built with SIM_MODE.

Requirements:
    pip install pyserial
"""

import tkinter as tk
from tkinter import ttk
import serial
import serial.tools.list_ports


FIELDS = [
    # (section, label, key, min, max, default, resolution)
    # -- Navigation --
    ("Navigation", "Page  (0-3)",         "page",       0,      3,     0,     1),
    ("Navigation", "Assist Level  (0-3)", "assist",     0,      3,     0,     1),

    # -- Dashboard --
    ("Dashboard",  "Speed  km/h",         "speed",      0,     80,    25,   0.5),
    ("Dashboard",  "SOC  %",              "soc",        0,    100,    75,   0.5),
    ("Dashboard",  "Power  W",            "power",   -500,   2000,   300,     5),
    ("Dashboard",  "Remaining Range  km", "range",      0,    100,    45,   0.5),

    # -- Power Metrics --
    ("Power",      "Input Voltage  V",    "voltage",   36,   67.2,    54,   0.1),
    ("Power",      "BMS Voltage  V",      "bms_volt",  36,   67.2,    54,   0.1),
    ("Power",      "Input Current  A",    "current",  -20,     40,     6,   0.1),
    ("Power",      "Motor Current  A",    "mot_cur",  -60,     60,    10,   0.5),
    ("Power",      "Duty Cycle",          "duty",      -1,      1,  0.15,  0.01),
    ("Power",      "Amp Hours  Ah",       "amp_hours",  0,     30,   2.5,   0.1),
    ("Power",      "Efficiency  Wh/km",   "effic",      0,     60,    20,   0.5),

    # -- Trip Computer --
    ("Trip",       "Trip Distance  km",   "trip",       0,    200,  12.3,   0.1),
    ("Trip",       "Odometer  km",        "odometer",   0,   9999,  1234,     1),
    ("Trip",       "Avg Speed  km/h",     "avg_speed",  0,     80,    18,   0.5),

    # -- System Health --
    ("Health",     "Motor Temp  °C",      "mot_temp",  -10,   120,    35,   0.5),
    ("Health",     "FET Temp  °C",        "fet_temp",  -10,   120,    42,   0.5),
    ("Health",     "BMS SOH  %",          "bms_soh",    0,    100,    98,   0.5),
    ("Health",     "CAN Load  msg/s",     "can_load",   0,    300,   120,     1),
    ("Health",     "BMS Cell Temp  °C",   "bms_temp",  -20,    60,    28,   0.5),
]

PRESETS = {
    "Idle": {
        "speed": 0, "power": 0, "current": 0, "mot_cur": 0,
        "duty": 0, "effic": 0, "range": 45, "soc": 75,
    },
    "Cruise 25": {
        "speed": 25, "power": 280, "current": 5.2, "mot_cur": 12,
        "duty": 0.18, "effic": 11.2, "range": 45,
    },
    "Max Power": {
        "speed": 45, "power": 1850, "current": 34, "mot_cur": 58,
        "duty": 0.92, "effic": 41, "range": 12,
    },
    "Regen": {
        "speed": 22, "power": -320, "current": -6, "mot_cur": -14,
        "duty": -0.2, "effic": 0,
    },
    "Low Battery": {
        "soc": 8, "range": 3, "speed": 20, "power": 220,
    },
    "Overheat": {
        "mot_temp": 112, "fet_temp": 96, "bms_temp": 48,
        "speed": 38, "power": 1400,
    },
}


class SimulatorApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("TROVATA Display — UI Simulator")
        self.root.geometry("580x820")
        self.ser: serial.Serial | None = None
        self.sliders: dict[str, tk.DoubleVar] = {}
        self._build_top()
        self._build_buttons()
        self._build_sliders()
        self._send_loop()

    # ------------------------------------------------------------------ build

    def _build_top(self):
        bar = tk.Frame(self.root, pady=6)
        bar.pack(fill="x", padx=8)

        tk.Label(bar, text="Port:").pack(side="left")
        self.port_var = tk.StringVar()
        self.port_cb = ttk.Combobox(bar, textvariable=self.port_var, width=10)
        self.port_cb.pack(side="left", padx=4)

        tk.Button(bar, text="Refresh", command=self._refresh_ports).pack(side="left", padx=2)
        self.conn_btn = tk.Button(bar, text="Connect", command=self._toggle_connect)
        self.conn_btn.pack(side="left", padx=2)

        self.status_var = tk.StringVar(value="Disconnected")
        self.status_lbl = tk.Label(bar, textvariable=self.status_var, fg="red", width=28, anchor="w")
        self.status_lbl.pack(side="left", padx=8)

        self._refresh_ports()

    def _build_buttons(self):
        panel = tk.Frame(self.root, padx=8, pady=4)
        panel.pack(fill="x")

        # --- Hardware buttons ---
        hw = tk.LabelFrame(panel, text="Hardware Buttons", padx=6, pady=4)
        hw.pack(side="left", fill="y", padx=(0, 6))

        btn_cfg = {"width": 10, "relief": "raised", "bd": 2}

        tk.Button(hw, text="B1  −Assist", bg="#d0e8ff", **btn_cfg,
                  command=self._b1_short).grid(row=0, column=0, padx=3, pady=2)
        tk.Button(hw, text="B1  Reset Trip", bg="#ffd0d0", **btn_cfg,
                  command=self._b1_long).grid(row=1, column=0, padx=3, pady=2)
        tk.Button(hw, text="B2  +Assist", bg="#d0e8ff", **btn_cfg,
                  command=self._b2_short).grid(row=0, column=1, padx=3, pady=2)
        tk.Button(hw, text="B2  Next Page", bg="#ffd0d0", **btn_cfg,
                  command=self._b2_long).grid(row=1, column=1, padx=3, pady=2)

        tk.Label(hw, text="short press", font=("", 7), fg="#888").grid(row=2, column=0, columnspan=1)
        tk.Label(hw, text="long press", font=("", 7), fg="#888").grid(row=2, column=1, columnspan=1)

        # --- Presets ---
        pre = tk.LabelFrame(panel, text="Presets", padx=6, pady=4)
        pre.pack(side="left", fill="both", expand=True)

        for i, (name, values) in enumerate(PRESETS.items()):
            col, row = i % 3, i // 3
            tk.Button(pre, text=name, width=10, relief="raised", bd=2,
                      command=lambda v=values: self._apply_preset(v)
                      ).grid(row=row, column=col, padx=3, pady=2)

    def _build_sliders(self):
        canvas = tk.Canvas(self.root, borderwidth=0)
        vsb = ttk.Scrollbar(self.root, orient="vertical", command=canvas.yview)
        canvas.configure(yscrollcommand=vsb.set)
        vsb.pack(side="right", fill="y")
        canvas.pack(side="left", fill="both", expand=True)

        frame = tk.Frame(canvas)
        frame.bind("<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.create_window((0, 0), window=frame, anchor="nw")
        canvas.bind("<MouseWheel>", lambda e: canvas.yview_scroll(int(-1 * e.delta / 120), "units"))

        current_section = None
        row = 0
        for section, label, key, lo, hi, default, res in FIELDS:
            if section != current_section:
                current_section = section
                sep = tk.Frame(frame, height=1, bg="#cccccc")
                sep.grid(row=row, column=0, columnspan=3, sticky="ew", padx=4, pady=(10, 0))
                row += 1
                tk.Label(frame, text=section, font=("", 9, "bold"), fg="#444444").grid(
                    row=row, column=0, columnspan=3, sticky="w", padx=6, pady=(0, 2))
                row += 1

            var = tk.DoubleVar(value=default)
            self.sliders[key] = var

            tk.Label(frame, text=label, width=22, anchor="w").grid(
                row=row, column=0, padx=(8, 2), pady=1, sticky="w")
            tk.Scale(frame, variable=var, from_=lo, to=hi, resolution=res,
                     orient=tk.HORIZONTAL, length=310, showvalue=False).grid(
                row=row, column=1, padx=2, pady=1)
            tk.Label(frame, textvariable=var, width=7, anchor="e").grid(
                row=row, column=2, padx=(2, 8), pady=1)
            row += 1

    # --------------------------------------------------------- button actions

    def _b1_short(self):
        v = self.sliders["assist"]
        v.set(max(0, v.get() - 1))

    def _b1_long(self):
        self.sliders["trip"].set(0)

    def _b2_short(self):
        v = self.sliders["assist"]
        v.set(min(3, v.get() + 1))

    def _b2_long(self):
        v = self.sliders["page"]
        v.set((int(v.get()) + 1) % 4)

    def _apply_preset(self, values: dict):
        for key, val in values.items():
            if key in self.sliders:
                self.sliders[key].set(val)

    # --------------------------------------------------------------- serial

    def _refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_cb["values"] = ports
        if ports:
            self.port_var.set(ports[0])

    def _toggle_connect(self):
        if self.ser and self.ser.is_open:
            self.ser.close()
            self.ser = None
            self.conn_btn.config(text="Connect")
            self._set_status("Disconnected", "red")
        else:
            try:
                self.ser = serial.Serial(self.port_var.get(), 115200, timeout=0.1)
                self.conn_btn.config(text="Disconnect")
                self._set_status(f"Connected  {self.port_var.get()}", "green")
            except Exception as exc:
                self._set_status(f"Error: {exc}", "red")

    def _set_status(self, msg: str, color: str):
        self.status_var.set(msg)
        self.status_lbl.config(fg=color)

    # ------------------------------------------------------------------ send

    def _send_loop(self):
        if self.ser and self.ser.is_open:
            payload = "".join(f"{k}={v.get():.3f}\n" for k, v in self.sliders.items())
            try:
                self.ser.write(payload.encode())
            except Exception:
                self._set_status("Send error — reconnect", "red")
                self.ser = None
                self.conn_btn.config(text="Connect")
        self.root.after(50, self._send_loop)  # 20 Hz


if __name__ == "__main__":
    root = tk.Tk()
    SimulatorApp(root)
    root.mainloop()
