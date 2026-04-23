# VESC Display 2 - UI Visualization

This document provides an ASCII representation of the 128x64 OLED display layouts for each page.

## Page 0: Dashboard (Main)
Focused on primary riding data with high visibility for speed and PAS level.

```text
+-----------------------------------------------------------+
| [ ]   . . . . . . .                                       |
| [ ] .             .                                       |
| [ ] .      2      .         25.4                          |
| [ ] .     PAS     .         km/h                          |
| [ ] .             .                                       |
| [ ]   . . . . . . .                                       |
| [ ]                                                       |
| 750W                          12.4km                      |
+-----------------------------------------------------------+
  ^           ^                  ^
  |           |                  |
Battery     PAS Gauge          Speed (Large)
```

## Page 1: Power Metrics
Detailed electrical performance and efficiency.

```text
+-----------------------------------------------------------+
|       . . . . .                                           |
|     .         .     V: 52.4V                              |
|     .   15    .     A: 12.2A                              |
|     .  Wh/km  .     P: 640W                               |
|       . . . . .                                           |
|                                                           |
|                     25 km/h                               |
+-----------------------------------------------------------+
  ^                    ^
  |                    |
Efficiency Gauge      Secondary Speed
```

## Page 2: Trip Computer
Navigation and session statistics.

```text
+-----------------------------------------------------------+
| TRIP COMPUTER                                             |
|-----------------------------------------------------------|
| Trip: 14.25 km                                            |
| Odo:  1240.2 km                                           |
| Time: 00:45:12          25 km/h                           |
| Avg:  18.5 km/h                                           |
+-----------------------------------------------------------+
```

## Page 3: System Health
Hardware temperatures and communication status.

```text
+-----------------------------------------------------------+
| SYSTEM HEALTH                                             |
|-----------------------------------------------------------|
| FET:   34.2 C                                             |
| Motor: 42.1 C                                             |
| BMS:   28.5 C           25 km/h                           |
| CAN:   120 msg/s                                          |
+-----------------------------------------------------------+
```

---

## Startup Animation
A 3-second sequence showing the brand and a moving cyclist.

```text
+-----------------------------------------------------------+
|                                                           |
|             TROVATA                                       |
|                                                           |
|                    __o                                    |
|                  _`\<,_                                   |
|                 (x)/ (x)                                  |
|         ~~~~~~~~~~~~~~~~~~~~~~                            |
+-----------------------------------------------------------+
```
