#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#define ARDUINOJSON_USE_LONG_LONG 1
#include "ArduinoJson.h"
#undef ARDUINOJSON_ENABLE_ARDUINO_STRING
#undef PAINLESSMESH_ENABLE_ARDUINO_STRING
#define PAINLESSMESH_ENABLE_STD_STRING
typedef std::string TSTRING;

#include "catch_utils.hpp"

#include "painlessmesh/buffer.hpp"

using namespace painlessmesh::buffer;

SCENARIO("ReceiveBuffer receives strings and needs to process them") {
  temp_buffer_t tmp_buffer;
  char cstring[3 * tmp_buffer.length];
  ReceiveBuffer<std::string> rBuffer = ReceiveBuffer<std::string>();

  GIVEN("A random string of short length pushed to the received buffer") {
    REQUIRE(rBuffer.empty());
    auto length = runif(10, tmp_buffer.length - 10);
    randomCString(cstring, length);
    // Note we need to send a \0 to know this is the end
    rBuffer.push(cstring, length + 1, tmp_buffer);
    THEN("It gets copied to the front of the buffer") {
      REQUIRE(!rBuffer.empty());
      REQUIRE(rBuffer.front() == std::string(cstring));
    }
  }

  GIVEN("A random string of long length pushed to the received buffer") {
    REQUIRE(rBuffer.empty());
    auto length = runif(tmp_buffer.length + 10, 2 * tmp_buffer.length);
    randomCString(cstring, length);
    // Note we need to send a \0 to know this is the end
    rBuffer.push(cstring, length + 1, tmp_buffer);
    THEN("It gets copied to the front of the buffer") {
      REQUIRE(!rBuffer.empty());
      REQUIRE(rBuffer.front() == std::string(cstring));
    }
  }

  GIVEN("A random string we can push it in multiple parts") {
    REQUIRE(rBuffer.empty());
    auto length = runif(tmp_buffer.length + 10, 2 * tmp_buffer.length);
    size_t part_len = length / 2;
    randomCString(cstring, length);
    rBuffer.push(cstring, part_len, tmp_buffer);
    THEN("The first part doesn't get copied to the front of the buffer") {
      REQUIRE(rBuffer.empty());
    }

    auto data_ptr = cstring + sizeof(char) * part_len;
    rBuffer.push(data_ptr, length - part_len + 1, tmp_buffer);
    THEN(
        "When getting the second part the whole thing gets copied to the front "
        "of the buffer") {
      REQUIRE(!rBuffer.empty());
      REQUIRE(rBuffer.front() == std::string(cstring));
    }
  }

  GIVEN("ReceiveBuffer can receive multiple messages and hold them all") {
    REQUIRE(rBuffer.empty());
    for (size_t i = 0; i < 10; ++i) {
      auto length = runif(tmp_buffer.length + 10, 2 * tmp_buffer.length);
      size_t part_len = length / 2;
      randomCString(cstring, length);
      rBuffer.push(cstring, part_len, tmp_buffer);
      auto data_ptr = cstring + sizeof(char) * part_len;
      rBuffer.push(data_ptr, length - part_len + 1, tmp_buffer);
    }
    THEN(
        "When getting the second part the whole thing gets copied to the front "
        "of the buffer") {
      REQUIRE(!rBuffer.empty());
      for (size_t i = 0; i < 10; ++i) {
        REQUIRE(!rBuffer.empty());
        rBuffer.pop_front();
      }
      REQUIRE(rBuffer.empty());
    }
  }

  GIVEN(
      "ReceiveBuffer can receive multiple messages in one char string "
      "(separated by \0") {
    REQUIRE(rBuffer.empty());
    auto length = runif(10, tmp_buffer.length - 10);
    randomCString(cstring, length);
    auto data_ptr = cstring + sizeof(char) * (length + 1);
    auto length2 = runif(10, tmp_buffer.length - 10);
    randomCString(data_ptr, length2);

    // Note we need to send a \0 to know this is the end
    rBuffer.push(cstring, length + length2 + 2, tmp_buffer);
    THEN("We have both strings in the buffer") {
      REQUIRE(!rBuffer.empty());
      REQUIRE(rBuffer.front() == std::string(cstring));
      rBuffer.pop_front();
      REQUIRE(!rBuffer.empty());
      REQUIRE(rBuffer.front() == std::string(data_ptr));
      rBuffer.pop_front();
      REQUIRE(rBuffer.empty());
    }
  }

  GIVEN(
      "ReceiveBuffer has copied the message we can overwrite the previous "
      "cstring and buffer without affecting the outcome") {
    REQUIRE(rBuffer.empty());
    cstring[0] = 'B';
    cstring[1] = 'l';
    cstring[2] = 'a';
    cstring[3] = 'a';
    cstring[4] = 't';
    cstring[5] = '\0';
    rBuffer.push(cstring, 3, tmp_buffer);
    cstring[0] = 'r';
    cstring[1] = 'n';
    cstring[2] = 'd';
    // randomCString writes a terminator at str[length], so at most
    // length - 1 fits in a buffer of tmp_buffer.length bytes
    randomCString(tmp_buffer.buffer, tmp_buffer.length - 1);
    auto data_ptr = cstring + sizeof(char) * 3;
    rBuffer.push(data_ptr, 3, tmp_buffer);
    THEN("We still have the correct result") {
      REQUIRE(rBuffer.front() == std::string("Blaat"));
      REQUIRE(std::string(cstring) != std::string("Blaat"));
      REQUIRE(std::string(tmp_buffer.buffer, 6) != std::string("Blaat"));
      REQUIRE(std::string(tmp_buffer.buffer, 5) != std::string("Blaat"));
    }
  }

  GIVEN("A buffer with multiple messages") {
    REQUIRE(rBuffer.empty());
    for (size_t i = 0; i < 10; ++i) {
      auto length = runif(tmp_buffer.length + 10, 2 * tmp_buffer.length);
      size_t part_len = length / 2;
      randomCString(cstring, length);
      rBuffer.push(cstring, part_len, tmp_buffer);
      auto data_ptr = cstring + sizeof(char) * part_len;
      rBuffer.push(data_ptr, length - part_len + 1, tmp_buffer);
    }
    THEN("We can clear it") {
      REQUIRE(!rBuffer.empty());
      rBuffer.clear();
      REQUIRE(rBuffer.empty());
    }
  }

  GIVEN("A buffer with a half written message") {
    REQUIRE(rBuffer.empty());
    for (size_t i = 0; i < 10; ++i) {
      auto length = runif(tmp_buffer.length + 10, 2 * tmp_buffer.length);
      size_t part_len = length / 2;
      randomCString(cstring, length);
      rBuffer.push(cstring, part_len, tmp_buffer);
      auto data_ptr = cstring + sizeof(char) * part_len;
      rBuffer.push(data_ptr, length - part_len + 1, tmp_buffer);
    }
    auto length = runif(tmp_buffer.length + 10, 2 * tmp_buffer.length);
    size_t part_len = length / 2;
    randomCString(cstring, length);
    rBuffer.push(cstring, part_len, tmp_buffer);

    THEN("We can clear removes it") {
      REQUIRE(!rBuffer.empty());
      rBuffer.clear();
      REQUIRE(rBuffer.empty());
    }
    rBuffer.clear();
    randomCString(cstring, length);
    rBuffer.push(cstring, length + 1, tmp_buffer);
    THEN("Reusing it works correctly") {
      REQUIRE(rBuffer.front() == std::string(cstring));
    }
  }
}

SCENARIO("SentBuffer receives strings and can be read in parts") {
  temp_buffer_t tmp_buffer;
  SentBuffer<std::string> sBuffer = SentBuffer<std::string>();
  GIVEN("A SentBuffer and a string") {
    auto length = runif(0, tmp_buffer.length - 10);
    auto msg = randomString(length);
    THEN("We can pass strings to it") {
      REQUIRE(sBuffer.empty());
      sBuffer.push(msg);
      REQUIRE(!sBuffer.empty());
    }
    THEN("We can pass it and read it back") {
      REQUIRE(sBuffer.empty());
      sBuffer.push(msg);
      REQUIRE(!sBuffer.empty());
      auto rlength = sBuffer.requestLength(2 * length);
      REQUIRE(rlength <= 2 * length);
      sBuffer.read(rlength, tmp_buffer);
      if (rlength == msg.length() + 1)
        REQUIRE(std::string(tmp_buffer.buffer, msg.length()) == msg);

      // Test free read as well
      sBuffer.freeRead();
      if (rlength == msg.length() + 1) REQUIRE(sBuffer.empty());
    }
  }

  // We can read in multiple parts
  GIVEN("A long string passed to the SentBuffer") {
    size_t length = runif(tmp_buffer.length + 10, 2 * tmp_buffer.length - 10);
    auto msg = randomString(length);
    sBuffer.push(msg);
    char cstring[length + 1];
    auto data_ptr = cstring;
    THEN("We can read it in multiple parts") {
      while (!sBuffer.empty()) {
        auto rlength = sBuffer.requestLength(tmp_buffer.length);
        sBuffer.read(rlength, tmp_buffer);
        memcpy(data_ptr, tmp_buffer.buffer, rlength);
        data_ptr += rlength * sizeof(char);
        sBuffer.freeRead();
      }
      REQUIRE(std::string(cstring) == msg);
    }

    THEN("We can use direct access to read it in multiple parts") {
      while (!sBuffer.empty()) {
        auto rlength = sBuffer.requestLength(tmp_buffer.length);
        auto ptr = sBuffer.readPtr(rlength);
        memcpy(data_ptr, ptr, rlength);
        data_ptr += rlength * sizeof(char);
        sBuffer.freeRead();
      }
      REQUIRE(std::string(cstring) == msg);
    }
  }

  GIVEN(
      "We have read a message halfway, priority messages can safely be added") {
    size_t length = runif(tmp_buffer.length + 10, 2 * tmp_buffer.length - 10);
    auto msg1 = randomString(length);
    auto msg2 = randomString(length);
    auto msgH = randomString(length);
    sBuffer.push(msg1);
    char cstring[3 * length + 3];
    auto data_ptr = cstring;
    THEN("We can read it in multiple parts") {
      auto rlength = sBuffer.requestLength(tmp_buffer.length);

      // Read first message halfway
      sBuffer.read(rlength, tmp_buffer);
      memcpy(data_ptr, tmp_buffer.buffer, rlength);
      data_ptr += rlength * sizeof(char);
      sBuffer.freeRead();
      sBuffer.push(msg2);
      sBuffer.push(msgH, true);

      // Read the rest of the messages
      while (!sBuffer.empty()) {
        rlength = sBuffer.requestLength(tmp_buffer.length);
        sBuffer.read(rlength, tmp_buffer);
        memcpy(data_ptr, tmp_buffer.buffer, rlength);
        data_ptr += rlength * sizeof(char);
        sBuffer.freeRead();
      }
      REQUIRE(std::string(cstring) == msg1);
      REQUIRE(std::string(cstring + length + 1) == msgH);
      REQUIRE(std::string(cstring + 2 * (length + 1)) == msg2);
    }
  }

  GIVEN("A SentBuffer with a message in it") {
    auto length = runif(0, tmp_buffer.length - 10);
    auto msg = randomString(length);
    sBuffer.push(msg);

    REQUIRE(!sBuffer.empty());
    THEN("Clear will empty it") {
      sBuffer.clear();
      REQUIRE(sBuffer.empty());
    }
  }
}

SCENARIO("requestLength never answers with more than it was asked for") {
  // The randomised scenario above draws its length from runif(0, ...), so it
  // asked for zero about once in sixty runs -- and CI failed there, on
  // `REQUIRE(rlength <= 2 * length)` expanding to `1 <= 0`, on pull requests
  // that had not been near the buffer. `buffer_length - 1` on an unsigned
  // zero is SIZE_MAX, so min() picked the message and a caller with no room
  // was told one byte was available.
  temp_buffer_t tmp_buffer;
  SentBuffer<std::string> sBuffer = SentBuffer<std::string>();

  GIVEN("an empty message in the buffer and a caller with no room") {
    sBuffer.push(std::string(""));
    REQUIRE(!sBuffer.empty());
    THEN("requestLength(0) is 0") { REQUIRE(sBuffer.requestLength(0) == 0); }
  }

  GIVEN("a message longer than the room offered") {
    sBuffer.push(std::string("0123456789"));
    THEN("the answer is bounded by the room, terminator included") {
      REQUIRE(sBuffer.requestLength(0) == 0);
      REQUIRE(sBuffer.requestLength(1) == 0);
      REQUIRE(sBuffer.requestLength(4) == 3);
      // Room to spare: the whole message and its terminator.
      REQUIRE(sBuffer.requestLength(tmp_buffer.length) == 11);
    }
  }

  GIVEN("nothing in the buffer") {
    THEN("every request is 0, whatever the room") {
      REQUIRE(sBuffer.requestLength(0) == 0);
      REQUIRE(sBuffer.requestLength(tmp_buffer.length) == 0);
    }
  }
}
