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

#include <memory>
#include <string>

#include <cobs_serial/serial.hpp>

#include "serial/serial.h"

namespace serial
{
class Serial;
}

namespace cobs_serial
{
class DefaultSerial : public Serial
{
public:
  /**
   * Creates a Serial object to send and receive bytes to and from the serial
   * port.
   */
  DefaultSerial();

  void open() override;

  [[nodiscard]] bool is_open() const override;

  void close() override;

  [[nodiscard]] std::size_t read(uint8_t* buffer, size_t size = 1) override;
  [[nodiscard]] std::size_t write(const uint8_t* buffer, size_t size) override;

  void set_port(const std::string& port) override;
  [[nodiscard]] std::string get_port() const override;

  void set_timeout(std::chrono::duration<double> timeout) override;
  [[nodiscard]] std::chrono::duration<double> get_timeout() const override;

  void set_baudrate(uint32_t baudrate) override;
  [[nodiscard]] uint32_t get_baudrate() const override;

private:
  std::unique_ptr<serial::Serial> serial_ = nullptr;
};
}  // namespace cobs_serial
