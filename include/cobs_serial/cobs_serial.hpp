// Copyright 2023 Giovanni Remigi
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cstdint>
#include <vector>

namespace cobs_serial
{
class CobsSerial
{
public:
  virtual ~CobsSerial() = default;

  virtual void open() = 0;
  virtual bool is_open() = 0;
  virtual void close() = 0;

  /**
   * Read a sequence of bytes from the serial port.
   * @return The bytes read.
   * @throw cobs_serial::SerialException
   */
  virtual std::vector<uint8_t> read() = 0;

  /**
   * Write a sequence of bytes to the serial port.
   * @param bytes The bytes to read.
   * @throw cobs_serial::SerialException
   */
  virtual void write(const std::vector<uint8_t>& bytes) = 0;
};
}  // namespace cobs_serial
