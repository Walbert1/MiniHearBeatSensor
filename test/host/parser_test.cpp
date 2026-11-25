// Host self-test for the LD2410 frame parser.
// Run: g++ -std=c++17 -Iinclude test/host/parser_test.cpp -o /tmp/pt && /tmp/pt
#include <cassert>
#include <cstdio>
#include "radar_sensor.h"

static int feedAll(LD2410Parser &p, const uint8_t *b, size_t n, RadarReading &out) {
  int frames = 0;
  for (size_t i = 0; i < n; i++) frames += p.feed(b[i], out);
  return frames;
}

int main() {
  // Basic frame: state 3, moving 240cm e50, still 100cm e20, detect 256cm.
  const uint8_t good[] = {0xF4,0xF3,0xF2,0xF1, 0x0D,0x00, 0x02,0xAA, 0x03, 0xF0,0x00, 0x32, 0x64,0x00, 0x14,
                          0x00,0x01, 0x55,0x00, 0xF8,0xF7,0xF6,0xF5};
  LD2410Parser p;
  RadarReading r{};

  const uint8_t junk[] = {0x00, 0xF4, 0xF4, 0xF3, 0x12};  // noise incl. a false header start
  assert(feedAll(p, junk, sizeof junk, r) == 0);
  assert(feedAll(p, good, sizeof good, r) == 1);
  assert(r.state == 3 && r.movingCm == 240 && r.movingEnergy == 50 && r.stillCm == 100 && r.stillEnergy == 20);

  uint8_t bad[sizeof good];  // corrupt footer -> rejected, then parser recovers
  for (size_t i = 0; i < sizeof good; i++) bad[i] = good[i];
  bad[sizeof bad - 1] = 0x00;
  assert(feedAll(p, bad, sizeof bad, r) == 0);
  assert(feedAll(p, good, sizeof good, r) == 1);

  const uint8_t ack[] = {0xFD,0xFC,0xFB,0xFA, 0x04,0x00, 0xFF,0x01,0x00,0x00, 0x04,0x03,0x02,0x01};  // command ACK ignored
  assert(feedAll(p, ack, sizeof ack, r) == 0);

  puts("parser ok");
}
