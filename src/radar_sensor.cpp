#include <Arduino.h>
#include "config.h"
#include "radar_sensor.h"

void RadarSensor::begin() {
  pinMode(PIN_RADAR_OUT, INPUT);
  Serial1.begin(RADAR_BAUD, SERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX);

  // Shorten the sensor's own presence hold. Saved in the LD2410, so re-sending each boot is harmless.
  // ACK replies use a different header (FD FC FB FA) and are ignored by the parser.
  static const uint8_t HDR[] = {0xFD, 0xFC, 0xFB, 0xFA}, END[] = {0x04, 0x03, 0x02, 0x01};
  auto cmd = [](const uint8_t *p, uint8_t len) {
    Serial1.write(HDR, 4); Serial1.write(len); Serial1.write((uint8_t)0);
    Serial1.write(p, len); Serial1.write(END, 4);
    delay(100);
  };
  const uint8_t enable[] = {0xFF, 0x00, 0x01, 0x00};
  const uint8_t params[] = {0x60, 0x00,                          // set max gates + unmanned duration
                            0x00, 0x00, 8, 0, 0, 0,              // max moving gate = 8 (6 m)
                            0x01, 0x00, 8, 0, 0, 0,              // max still gate = 8
                            0x02, 0x00, RADAR_NOONE_S, 0, 0, 0}; // unmanned duration (s)
  const uint8_t finish[] = {0xFE, 0x00};
  cmd(enable, sizeof enable);
  cmd(params, sizeof params);
  cmd(finish, sizeof finish);
}

void RadarSensor::update() {
  RadarReading tmp;
  while (Serial1.available()) {
    bytes++;
    if (parser.feed(Serial1.read(), tmp)) {
      r = tmp;
      lastFrameMs = millis();
      gotFrame = true;
    }
  }
}

bool RadarSensor::online() const { return gotFrame && millis() - lastFrameMs < SENSOR_TIMEOUT_MS; }

bool RadarSensor::outPin() const { return digitalRead(PIN_RADAR_OUT) == HIGH; }
