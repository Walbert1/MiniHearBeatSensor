#pragma once
// All pins and tunable settings live here.

// ---------- LD2410 radar ----------
#define PIN_RADAR_OUT   0       // digital presence output, HIGH = presence
#define PIN_RADAR_RX    20      // ESP32 RX  <- LD2410 TX
#define PIN_RADAR_TX    21      // ESP32 TX  -> LD2410 RX
#define RADAR_BAUD      256000

// ---------- TFT (hardware SPI) ----------
#define PIN_TFT_SCK     4
#define PIN_TFT_MOSI    6
#define PIN_TFT_CS      7
#define PIN_TFT_DC      10
#define PIN_TFT_RST     3
#define PIN_TFT_BL      1       // backlight, PWM, HIGH = on

// Pick your driver: 0 = ST7735 (128x160), 1 = ST7789.
#define TFT_USE_ST7789  1       // 1.54" 240x240 panel
// ST7789 only: native panel size (portrait), e.g. 240x240, 240x320, 240x280, 135x240.
#define TFT_WIDTH       240
#define TFT_HEIGHT      240
// ST7735 only: init variant. Try INITR_GREENTAB / INITR_REDTAB if colors or edges look wrong.
#define TFT_ST7735_TAB  INITR_BLACKTAB
#define TFT_ROTATION    3       // 1 or 3 = landscape (3 = flipped)
#define TFT_SPI_HZ      27000000 // drop to 16000000 if you see garbage
#define TFT_BRIGHTNESS  200     // backlight PWM 0-255

// ---------- Radar look / behavior ----------
#define RADAR_MAX_RANGE_CM 600  // LD2410 default max range (6 m)
#define RADAR_RING_COUNT   3    // range rings (3 -> 2/4/6 m)
#define SENSOR_TIMEOUT_MS  2000 // no valid frame this long -> SENSOR ERROR
#define CLEAR_HOLD_MS      300  // keep showing a target this long after it disappears
#define CLEAR_FADE_MS      300  // then fade the dot out over this long
#define RADAR_SPLIT_CM     75   // moving vs still target this far apart -> show as 2 people
#define RADAR_NOONE_S      1    // LD2410 "unmanned duration": seconds it keeps reporting presence (factory 5)
#define FRAME_MS           40   // ~25 FPS
#define DEBUG_PRINT_MS     500  // USB serial debug print interval
