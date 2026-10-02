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

#include <cstdint>

#include <cobs_serial/default_cobs_serial.hpp>

#include <cobs_serial/crc_utils.hpp>
#include <cobs_serial/data_utils.hpp>
#include <cobs_serial/serial_exception.hpp>

#include <rclcpp/logging.hpp>

namespace cobs_serial
{
constexpr auto kLogger = "DataHandler";

constexpr uint8_t DELIMITER_FLAG = 0x7E;  // Start and end of a packet
constexpr uint8_t ESCAPE_FLAG = 0x7D;     // Escaping byte.
constexpr uint8_t ESCAPED_XOR = 0x20;     // XOR value applied to escaped bytes.

void add_escaped_byte(Buffer<uint8_t>& buffer, uint8_t ch)
{
  if (ch == ESCAPE_FLAG || ch == DELIMITER_FLAG)
  {
    buffer.add(ESCAPE_FLAG, BufferPosition::Tail);
    buffer.add(ch ^ ESCAPED_XOR, BufferPosition::Tail);
  }
  else
  {
    buffer.add(ch, BufferPosition::Tail);
  }
}

DefaultCobsSerial::DefaultCobsSerial(std::unique_ptr<Serial> serial) : serial_{ std::move(serial) }
{
}

void DefaultCobsSerial::open()
{
  serial_->open();
}

bool cobs_serial::DefaultCobsSerial::is_open()
{
  return serial_->is_open();
}

void DefaultCobsSerial::close()
{
  serial_->close();
}

std::vector<uint8_t> DefaultCobsSerial::read()
{
  read_buffer_.clear();

  uint16_t crc = 0;
  state_ = ReadState::Waiting;
  uint8_t ch = 0;

  while (true)
  {
    if (serial_->read(&ch, 1) == 0)
    {
      throw SerialException("timeout");
    }

    if (state_ == ReadState::Waiting)
    {
      if (ch == DELIMITER_FLAG)
      {
        state_ = ReadState::ReadingMessage;
      }
      else
      {
        throw SerialException("start delimiter missing");
      }
    }
    else if (state_ == ReadState::ReadingMessage)
    {
      if (ch == ESCAPE_FLAG)
      {
        state_ = ReadState::ReadingEscapedByte;
      }
      else if (ch == DELIMITER_FLAG)
      {
        if (read_buffer_.size() < 3)
        {
          // We expect at least one byte of data and two bytes for the CRC.
          throw SerialException("incorrect frame length");
        }
        else if (crc != 0)
        {
          std::vector<uint8_t> data;
          while (read_buffer_.size() > 0)
          {
            data.emplace_back(read_buffer_.remove(BufferPosition::Head));
          }

          RCLCPP_ERROR(rclcpp::get_logger(kLogger), "CRC error on data: %s", data_utils::to_hex(data).c_str());
          throw SerialException("CRC error");
        }
        else
        {
          // Remove CRC from buffer.
          read_buffer_.remove(BufferPosition::Tail);
          read_buffer_.remove(BufferPosition::Tail);
          break;
        }
      }
      else
      {
        crc = crc_ccitt_byte(crc, ch);
        read_buffer_.add(ch, BufferPosition::Tail);
      }
    }
    else if (state_ == ReadState::ReadingEscapedByte)
    {
      crc = crc_ccitt_byte(crc, ch ^ ESCAPED_XOR);
      read_buffer_.add(ch ^ ESCAPED_XOR, BufferPosition::Tail);
      state_ = ReadState::ReadingMessage;
    }
  }

  std::vector<uint8_t> buffer;
  buffer.reserve(read_buffer_.size());
  while (read_buffer_.size() > 0)
  {
    buffer.emplace_back(read_buffer_.remove(BufferPosition::Head));
  }

  return buffer;
}

void DefaultCobsSerial::write(const std::vector<uint8_t>& buffer)
{
  write_buffer_.clear();
  uint16_t crc = 0;

  write_buffer_.add(DELIMITER_FLAG, BufferPosition::Tail);
  for (const uint8_t& ch : buffer)
  {
    crc = crc_ccitt_byte(crc, ch);
    add_escaped_byte(write_buffer_, ch);
  }
  const uint8_t crc_lsb = static_cast<uint8_t>((crc & 0xff00) >> 8);
  const uint8_t crc_msb = static_cast<uint8_t>((crc & 0x00ff));
  add_escaped_byte(write_buffer_, crc_msb);
  add_escaped_byte(write_buffer_, crc_lsb);
  write_buffer_.add(DELIMITER_FLAG, BufferPosition::Tail);

  while (write_buffer_.size() > 0)
  {
    const uint8_t ch = write_buffer_.remove(BufferPosition::Head);
    std::size_t size = serial_->write(&ch, 1);
    if (size == 0)
    {
      throw SerialException("error writing to the serial port");
    }
  }
}

}  // namespace cobs_serial
