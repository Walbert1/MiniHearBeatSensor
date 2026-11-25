// ESP32-C3 Super Mini + HLK-LD2410 radar -> radar-style TFT display.
#include <Arduino.h>
#include "config.h"
#include "radar_sensor.h"
#include "radar_display.h"

static RadarSensor radar;
static RadarDisplay display;

void setup() {
  Serial.begin(115200);  // USB CDC
  radar.begin();
  display.begin();
  Serial.println("LD2410 radar display started");
}

void loop() {
  static uint32_t lastFrame = 0, lastPrint = 0;
  radar.update();  // non-blocking: parses whatever bytes have arrived

  const uint32_t now = millis();
  if (now - lastFrame >= FRAME_MS) {
    lastFrame = now;
    display.draw(radar);
  }
  if (now - lastPrint >= DEBUG_PRINT_MS) {
    lastPrint = now;
    const RadarReading &r = radar.reading();
    Serial.printf("rx=%lu online=%d out=%d state=%u moving=%ucm e=%u still=%ucm e=%u\n",
                  (unsigned long)radar.rxBytes(), radar.online(), radar.outPin(), r.state,
                  r.movingCm, r.movingEnergy, r.stillCm, r.stillEnergy);
  }
}
