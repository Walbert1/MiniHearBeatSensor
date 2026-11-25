#pragma once
#include <stdint.h>
#include <stddef.h>

// One decoded LD2410 target report. Distance only: the LD2410 does not report angle.
struct RadarReading {
  uint8_t  state;         // 0 = none, 1 = moving, 2 = stationary, 3 = both
  uint16_t movingCm;
  uint8_t  movingEnergy;  // 0-100
  uint16_t stillCm;
  uint8_t  stillEnergy;   // 0-100
};

// Byte-at-a-time parser for LD2410 report frames (basic and engineering mode).
// Frame: F4 F3 F2 F1 | len(2, LE) | type AA state mDist(2) mE sDist(2) sE ... | F8 F7 F6 F5
// No Arduino dependencies so it can be tested on the host (test/host/parser_test.cpp).
class LD2410Parser {
public:
  // Returns true when a complete, valid frame was decoded into `out`.
  bool feed(uint8_t b, RadarReading &out) {
    static const uint8_t HDR[4] = {0xF4, 0xF3, 0xF2, 0xF1};
    if (n < 4) {                       // still hunting for the header
      if (b == HDR[n]) buf[n++] = b;
      else if (b == HDR[0]) { buf[0] = b; n = 1; }
      else n = 0;
      return false;
    }
    buf[n++] = b;
    if (n < 6) return false;
    size_t len = buf[4] | (buf[5] << 8);
    if (len < 13 || 6 + len + 4 > sizeof(buf)) { n = 0; return false; }  // bogus length, resync
    if (n < 6 + len + 4) return false;
    n = 0;
    const uint8_t *d = buf + 6, *f = d + len;
    if (f[0] != 0xF8 || f[1] != 0xF7 || f[2] != 0xF6 || f[3] != 0xF5) return false;
    if ((d[0] != 0x01 && d[0] != 0x02) || d[1] != 0xAA) return false;
    out.state        = d[2];
    out.movingCm     = d[3] | (d[4] << 8);
    out.movingEnergy = d[5];
    out.stillCm      = d[6] | (d[7] << 8);
    out.stillEnergy  = d[8];
    return true;
  }
private:
  uint8_t buf[64];
  size_t n = 0;
};

// Non-blocking LD2410 reader on Serial1 plus the OUT pin.
class RadarSensor {
public:
  void begin();
  void update();                 // drain whatever UART bytes are waiting; never blocks
  bool online() const;           // a valid frame arrived within SENSOR_TIMEOUT_MS
  bool outPin() const;           // OUT pin HIGH = presence (fallback/confirmation)
  const RadarReading &reading() const { return r; }
  uint32_t rxBytes() const { return bytes; }  // raw UART bytes seen, for wiring diagnosis
private:
  LD2410Parser parser;
  RadarReading r{};
  uint32_t lastFrameMs = 0;
  bool gotFrame = false;
  uint32_t bytes = 0;
};
