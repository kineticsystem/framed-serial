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
#include <memory>
#include <string>
#include <vector>

#include <cobs_serial/buffer.hpp>
#include <cobs_serial/cobs_serial.hpp>
#include <cobs_serial/serial.hpp>

namespace cobs_serial
{
/**
 * This class is used to pack a sequence of bytes into a frame and send it to
 * the serial port and also to parse frames coming from the serial port.
 * A frames contains the data, a 16-bits CRC and delimiters.
 */
class DefaultCobsSerial : public CobsSerial
{
public:
  explicit DefaultCobsSerial(std::unique_ptr<Serial> serial);

  /**
   * Open the serial connection.
   */
  void open() override;

  /**
   * Returns true if the serial connection is open.
   */
  bool is_open() override;

  /**
   * Close the serial connection.
   */
  void close() override;

  /**
   * Write a sequence of bytes to the serial port.
   * @param bytes The bytes to read.
   * @throw cobs_serial::SerialException
   */
  void write(const std::vector<uint8_t>& bytes) override;

  /**
   * Read a sequence of bytes from the serial port.
   * @return The bytes read.
   * @throw cobs_serial::SerialException
   */
  std::vector<uint8_t> read() override;

private:
  /* States used while reading and parsiong a frame. */
  enum class ReadState
  {
    Waiting,
    ReadingMessage,
    ReadingEscapedByte
  } state_ = ReadState::Waiting;

  /* Circular buffer to read data from the serial port. */
  Buffer<uint8_t> read_buffer_{ 256 };

  /* Circular buffer to write data to the serial port. */
  Buffer<uint8_t> write_buffer_{ 256 };

  std::unique_ptr<Serial> serial_;
};
}  // namespace cobs_serial
