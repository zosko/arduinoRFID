/**
 *  This file is available under the terms of the MIT License, see the LICENSE file for full details
 */

#include "Int64String.h"

// Unsigned integer function, it does the bulk of the work, returns a String
// Based on work done by Rob Tillaart on the print9 function from:
// https://forum.arduino.cc/index.php?topic=143584.msg1519824#msg1519824
String int64String(uint64_t value, uint8_t base, bool prefix, bool sign) {
  // keep base within "reasonable limits
  if (base < 2)
    base = 2;
  else if (base > 16)
    base = 16;

  // start at position 64 (65th byte) and work backwards
  uint8_t i = 64;
  // 66 buffer for 64 characters (binary) + B prefix + \0
  char buffer[66] = {0};

  if (value == 0)
    buffer[i--] = '0';
  else {
    uint8_t base_multiplied = 3;
    uint16_t multiplier = base * base * base;

    if (base < 16) {
      multiplier *= base;
      base_multiplied++;
    }
    if (base < 8) {
      multiplier *= base;
      base_multiplied++;
    }

    // Five 64 bit devisions max
    while (value > multiplier) {
      uint64_t q = value / multiplier;
      // get remainder without doing another division with %
      uint16_t r = value - q * multiplier;

      for (uint8_t j = 0; j < base_multiplied; j++) {
        uint16_t rq = r / base;
        buffer[i--] = base16char(r - rq * base);
        r = rq;
      }

      value = q;
    }

    uint16_t remaining = value;
    while (remaining > 0) {
      uint16_t q = remaining / base;
      buffer[i--] = base16char(remaining - q * base);
      remaining = q;
    }
  }

  // Signed numbers only make sense for decimal numbers
  if (base == DEC && sign)
    buffer[i--] = '-';
  else if (prefix) {
    if (base == HEX) {
      // 0x prefix
      buffer[i--] = 'x';
      buffer[i--] = '0';
    }
    else if (base == OCT)
      buffer[i--] = '0';
    else if (base == BIN)
      buffer[i--] = 'B';
  }

  // return String starting at position i + 1
  return String(&buffer[i + 1]);
}

// Signed integer function, returns a String
String int64String(int64_t value, uint8_t base, bool prefix) {
  // Signed numbers only make sense for decimal numbers
  bool sign = base == DEC && value < 0;
  // if signed, make it positive
  uint64_t uvalue = sign ? -value : value;

  // call the unsigned function to format the number
  return int64String(uvalue, base, prefix, sign);
}