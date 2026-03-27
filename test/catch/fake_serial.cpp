/**
 * Fake Serial implementation for desktop testing of painlessMesh.
 * Provides an Arduino-compatible Serial interface that writes to stdout.
 */

#include <cstring>
#include <iomanip>
#include <iostream>

#include "fake_serial.hpp"

void FakeSerial::begin(unsigned long speed) { return; }

void FakeSerial::end() { return; }

size_t FakeSerial::write(const unsigned char buf[], size_t size) {
  using namespace std;
  ios_base::fmtflags oldFlags = cout.flags();
  streamsize oldPrec = cout.precision();
  char oldFill = cout.fill();

  cout << "Serial::write: ";
  cout << internal << setfill('0');

  for (unsigned int i = 0; i < size; i++) {
    cout << setw(2) << hex << (unsigned int)buf[i] << " ";
  }
  cout << endl;

  cout.flags(oldFlags);
  cout.precision(oldPrec);
  cout.fill(oldFill);

  return size;
}

void FakeSerial::print(const char* buf) { std::cout << buf; }

void FakeSerial::println() { std::cout << std::endl; }

FakeSerial Serial;
