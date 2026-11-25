# Mini Heartbeat Sensor

ESP32-C3 Super Mini + HLK-LD2410 presence radar + 1.54" ST7789 240x240 TFT.
Shows a radar layout with a blue dot at the distance of each detected person.

## Wiring

```
        HLK-LD2410                ESP32-C3 Super Mini              1.54" ST7789 TFT
     ┌──────────────┐            ┌───────────────────┐            ┌──────────────┐
     │          VCC ├────────────┤ 5V                │            │              │
     │          GND ├────────────┤ GND ──────────────┼────────────┤ GND          │
     │           TX ├───────────►│ GPIO20 (RX)       │            │              │
     │           RX │◄───────────┤ GPIO21 (TX)       │            │              │
     │          OUT ├───────────►│ GPIO0             │            │              │
     └──────────────┘            │               3V3 ├────────────┤ VCC          │
                                 │  GPIO4  (SCK)     ├───────────►│ SCL / SCK    │
                                 │  GPIO6  (MOSI)    ├───────────►│ SDA / MOSI   │
                                 │  GPIO3            ├───────────►│ RES / RST    │
                                 │  GPIO10           ├───────────►│ DC           │
                                 │  GPIO7            ├───────────►│ CS           │
                                 │  GPIO1            ├───────────►│ BLK / BL     │
                                 └───────────────────┘            └──────────────┘
```

| From            | Pin  | To ESP32-C3 | Notes                                  |
|-----------------|------|-------------|----------------------------------------|
| LD2410          | VCC  | 5V          | Needs 5 V, 3.3 V is not enough          |
| LD2410          | GND  | GND         |                                        |
| LD2410          | TX   | GPIO20      | Sensor TX → ESP RX                     |
| LD2410          | RX   | GPIO21      | Sensor RX ← ESP TX                     |
| LD2410          | OUT  | GPIO0       | HIGH = presence                        |
| TFT             | VCC  | 3V3         |                                        |
| TFT             | GND  | GND         |                                        |
| TFT             | SCL  | GPIO4       | SPI clock                              |
| TFT             | SDA  | GPIO6       | SPI data (MOSI)                        |
| TFT             | RES  | GPIO3       | Reset                                  |
| TFT             | DC   | GPIO10      | Data / command                         |
| TFT             | CS   | GPIO7       | Chip select                            |
| TFT             | BLK  | GPIO1       | Backlight (PWM)                        |

All grounds must be connected together. Pins are set in [include/config.h](include/config.h).

## Build and flash

```
pio run -t upload -t monitor
```

The serial monitor prints `rx=<bytes> online=<0/1> ...`. If `rx` stays at 0, check the
LD2410 power and TX/RX wiring (try swapping TX and RX).

## Limits

The LD2410 reports distance only, not direction, so the dots sit on the centre line.
It can show a second person only when one is moving and the other is still, at least
`RADAR_SPLIT_CM` apart. For reliable tracking of 2–3 people, use an HLK-LD2450.
