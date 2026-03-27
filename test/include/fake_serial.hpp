/**
 * Fake Serial implementation for desktop testing of painlessMesh.
 * Provides an Arduino-compatible Serial interface that writes to stdout.
 */
#pragma once

#include <iostream>

class FakeSerial {
 public:
  void begin(unsigned long);
  void end();
  size_t write(const unsigned char*, size_t);
  void print(const char*);
  void println();
};

extern FakeSerial Serial;
