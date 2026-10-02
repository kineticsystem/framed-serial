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

#include <cobs_serial/default_serial.hpp>

#include <serial/serial.h>

namespace cobs_serial
{
DefaultSerial::DefaultSerial() : serial_{ std::make_unique<serial::Serial>() }
{
}

void DefaultSerial::open()
{
  serial_->open();
}

bool DefaultSerial::is_open() const
{
  return serial_->isOpen();
}

void DefaultSerial::close()
{
  serial_->close();
}

std::size_t DefaultSerial::read(uint8_t* buffer, size_t size)
{
  return serial_->read(buffer, size);
}

std::size_t DefaultSerial::write(const uint8_t* buffer, size_t size)
{
  std::size_t write_size = serial_->write(buffer, size);
  serial_->flush();
  return write_size;
}

void DefaultSerial::set_port(const std::string& port)
{
  serial_->setPort(port);
}

std::string DefaultSerial::get_port() const
{
  return serial_->getPort();
}

void DefaultSerial::set_timeout(std::chrono::duration<double> timeout)
{
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(timeout);
  serial::Timeout simple_timeout = serial::Timeout::simpleTimeout(static_cast<uint32_t>(millis.count()));
  serial_->setTimeout(simple_timeout);
}

std::chrono::duration<double> DefaultSerial::get_timeout() const
{
  std::chrono::milliseconds duration(serial_->getTimeout().read_timeout_constant);
  return duration;
}

void DefaultSerial::set_baudrate(uint32_t baudrate)
{
  serial_->setBaudrate(baudrate);
}

uint32_t DefaultSerial::get_baudrate() const
{
  return serial_->getBaudrate();
}
}  // namespace cobs_serial
