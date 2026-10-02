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

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace cobs_serial
{
class Serial
{
public:
  virtual ~Serial() = default;

  /**
   * Opens the serial port as long as the port is set and the port isn't
   * already open.
   */
  virtual void open() = 0;

  /**
   * Gets the open status of the serial port.
   * @return Returns true if the port is open, false otherwise.
   */
  [[nodiscard]] virtual bool is_open() const = 0;

  /** Closes the serial port. */
  virtual void close() = 0;

  /**
   * Read a given amount of bytes from the serial port into a give buffer.
   *
   * @param buffer The buffer.
   * @param size A size_t defining how many bytes to be read.
   * @return A size_t representing the number of bytes read as a result of the
   *         call to read.
   * @throw serial::PortNotOpenedException
   * @throw serial::SerialException
   */
  [[nodiscard]] virtual std::size_t read(uint8_t* buffer, size_t size) = 0;

  /**
   * Write a sequence of bytes to the serial port.
   * @param  data A const reference containing the data to be written
   * to the serial port.
   * @param  A size_t representing the number of bytes actually written to
   * the serial port.
   * @throw serial::PortNotOpenedException
   * @throw serial::SerialException
   * @throw serial::IOException
   */
  [[nodiscard]] virtual std::size_t write(const uint8_t* buffer, size_t size) = 0;

  /**
   * Sets the serial port identifier.
   * @param port A const std::string reference containing the address of the
   * serial port, which would be something like "COM1" on Windows and
   * "/dev/ttyS0" on Linux.
   * @throw std::invalid_argument
   */
  virtual void set_port(const std::string& port) = 0;

  /**
   * Gets the serial port identifier.
   * @see SerialInterface::setPort
   * @throw std::invalid_argument
   */
  [[nodiscard]] virtual std::string get_port() const = 0;

  /**
   * Set read timeout in seconds.
   * @param timeout Read timeout in seconds.
   */
  virtual void set_timeout(std::chrono::duration<double> timeout) = 0;

  /**
   * Get read timeout in seconds.
   * @return Read timeout in seconds.
   */
  [[nodiscard]] virtual std::chrono::duration<double> get_timeout() const = 0;

  /**
   * Sets the baudrate for the serial port.
   * @param baudrate An integer that sets the baud rate for the serial port.
   */
  virtual void set_baudrate(uint32_t baudrate) = 0;

  /**
   * Gets the baudrate for the serial port.
   * @return An integer that sets the baud rate for the serial port.
   */
  [[nodiscard]] virtual uint32_t get_baudrate() const = 0;
};
}  // namespace cobs_serial
