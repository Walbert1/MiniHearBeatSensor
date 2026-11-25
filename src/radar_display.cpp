#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include "config.h"
#include "radar_display.h"

#if TFT_USE_ST7789
#include <Adafruit_ST7789.h>
static Adafruit_ST7789 tft(&SPI, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
#else
#include <Adafruit_ST7735.h>
static Adafruit_ST7735 tft(&SPI, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
#endif

static GFXcanvas16 *cv = nullptr;
static int16_t W, H, cx, cy, R;           // screen size, radar center, radar radius
static const int16_t BAR_H = 12;          // top status bar height

// Last detected target, kept for the hold + fade after it disappears.
static RadarReading hold{};
static uint32_t lastSeenMs = 0;

// RGB (0-255) scaled by brightness a (0-1) -> RGB565.
static uint16_t rgb(float r, float g, float b, float a = 1.0f) {
  a = constrain(a, 0.0f, 1.0f);
  uint8_t R8 = r * a, G8 = g * a, B8 = b * a;
  return ((R8 & 0xF8) << 8) | ((G8 & 0xFC) << 3) | (B8 >> 3);
}

static int16_t radiusFor(uint16_t cm) { return (int32_t)R * min<int>(cm, RADAR_MAX_RANGE_CM) / RADAR_MAX_RANGE_CM; }

// Point on the semicircle: angle 0 = right, PI = left.
static int16_t px(float ang, int16_t r) { return cx + r * cosf(ang); }
static int16_t py(float ang, int16_t r) { return cy - r * sinf(ang); }

static void centeredText(const char *s, int16_t y, uint16_t color) {
  cv->setTextColor(color);
  cv->setCursor((W - (int16_t)strlen(s) * 6) / 2, y);
  cv->print(s);
}

bool RadarDisplay::begin() {
  pinMode(PIN_TFT_BL, OUTPUT);
  setBrightness(TFT_BRIGHTNESS);
  SPI.begin(PIN_TFT_SCK, -1, PIN_TFT_MOSI, PIN_TFT_CS);
#if TFT_USE_ST7789
  tft.init(TFT_WIDTH, TFT_HEIGHT);
#else
  tft.initR(TFT_ST7735_TAB);
#endif
  tft.setSPISpeed(TFT_SPI_HZ);
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(0);

  W = tft.width();
  H = tft.height();
  cv = new GFXcanvas16(W, H);
  if (!cv->getBuffer()) {
    tft.setTextColor(0xF800);
    tft.setCursor(2, 2);
    tft.print("NO RAM FOR CANVAS");
    Serial.println("Canvas allocation failed");
    return false;
  }
  cx = W / 2;
  cy = H - 2;
  R = min<int16_t>(W / 2 - 2, cy - BAR_H - 3);
  return true;
}

void RadarDisplay::setBrightness(uint8_t level) { analogWrite(PIN_TFT_BL, level); }

void RadarDisplay::draw(const RadarSensor &s) {
  if (!cv || !cv->getBuffer()) return;
  const uint32_t now = millis();
  const RadarReading &r = s.reading();
  const bool online = s.online();

  // --- Target memory: frames give distances, OUT pin can extend presence ---
  if (online && r.state) { hold = r; lastSeenMs = now; }
  else if (s.outPin() && hold.state) lastSeenMs = now;  // OUT confirms someone is still there
  const uint32_t since = now - lastSeenMs;
  const bool shown = hold.state && since < CLEAR_HOLD_MS;
  float fade = !hold.state ? 0
             : since < CLEAR_HOLD_MS ? 1
             : 1.0f - float(since - CLEAR_HOLD_MS) / CLEAR_FADE_MS;
  fade = max(fade, 0.0f);

  cv->fillScreen(0);

  // 2. Grid: range rings with labels, spokes every 30 degrees, baseline.
  const uint16_t grid = rgb(0, 50, 130), label = rgb(60, 140, 230);
  cv->setTextSize(1);
  for (int i = 1; i <= RADAR_RING_COUNT; i++) {
    int16_t rr = R * i / RADAR_RING_COUNT;
    cv->drawCircleHelper(cx, cy, rr, 0x3, grid);
    char buf[16];
    snprintf(buf, sizeof buf, "%gm", RADAR_MAX_RANGE_CM / 100.0f * i / RADAR_RING_COUNT);
    cv->setTextColor(label);
    cv->setCursor(cx + rr - (int16_t)strlen(buf) * 6 - 1, cy - 9);
    cv->print(buf);
  }
  for (int deg = 30; deg < 180; deg += 30) {
    float a = deg * DEG_TO_RAD;
    cv->drawLine(cx, cy, px(a, R), py(a, R), rgb(0, 30, 90));
  }
  cv->drawFastHLine(cx - R, cy, 2 * R + 1, grid);

  // 3. Presence dots at the target distance, straight ahead (LD2410 gives distance, not angle).
  //    shortcut: a 2nd dot only when moving and still targets are far apart; can miss or double-count, LD2450 for real multi-target.
  const bool two = (hold.state == 3) && abs(int(hold.movingCm) - int(hold.stillCm)) >= RADAR_SPLIT_CM;
  auto dot = [&](uint16_t cm, bool moving) {
    const float pulse = moving ? 0.5f + 0.5f * sinf(now * (TWO_PI / 700.0f)) : 0.0f;
    const int16_t x = px(HALF_PI, radiusFor(cm)), y = py(HALF_PI, radiusFor(cm));
    const int16_t d = max(4, R / 16);
    cv->fillCircle(x, y, d + 3 + int(pulse * 3), rgb(0, 90, 255, fade * 0.35f));  // glow
    cv->fillCircle(x, y, d, rgb(40, 140, 255, fade));
    cv->fillCircle(x, y, d / 2, rgb(180, 220, 255, fade * (0.6f + 0.4f * pulse))); // bright core
  };
  if (fade > 0 && hold.state) {
    if (hold.state & 1) dot(hold.movingCm, true);
    if (two || hold.state == 2) dot(hold.stillCm, false);
  }

  // 5. Status bar.
  char txt[24];
  uint16_t col;
  if (!online)                 { snprintf(txt, sizeof txt, "SENSOR ERROR"); col = rgb(255, 40, 40); }
  else if (!shown)             { snprintf(txt, sizeof txt, "CLEAR");        col = rgb(60, 150, 255); }
  else if (two)                { snprintf(txt, sizeof txt, "2 TARGETS %.1f/%.1fm", hold.movingCm / 100.0f, hold.stillCm / 100.0f); col = rgb(150, 210, 255); }
  else if (hold.state & 1)     { snprintf(txt, sizeof txt, "MOVING %.1fm", hold.movingCm / 100.0f); col = rgb(150, 210, 255); }
  else                         { snprintf(txt, sizeof txt, "STILL %.1fm",  hold.stillCm / 100.0f);  col = rgb(0, 120, 255); }
  cv->setTextColor(col);
  cv->setCursor(2, 2);
  cv->print(txt);
  cv->setTextColor(s.outPin() ? rgb(60, 150, 255) : rgb(0, 30, 90));
  cv->setCursor(W - 3 * 6 - 2, 2);
  cv->print("OUT");
  cv->drawFastHLine(0, BAR_H, W, grid);

  if (!online && (now / 500) % 2)  // blinking warning in the middle of the radar
    centeredText("SENSOR ERROR", cy - R / 2, rgb(255, 40, 40));

  // 6. Push the whole frame in one go (no flicker).
  tft.drawRGBBitmap(0, 0, cv->getBuffer(), W, H);
}
