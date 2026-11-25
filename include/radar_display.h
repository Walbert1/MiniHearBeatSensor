#pragma once
#include "radar_sensor.h"

// Draws the radar screen into an off-screen canvas and pushes it to the TFT each frame.
class RadarDisplay {
public:
  bool begin();                       // false if the frame buffer could not be allocated
  void setBrightness(uint8_t level);  // backlight PWM 0-255
  void draw(const RadarSensor &sensor);
};
