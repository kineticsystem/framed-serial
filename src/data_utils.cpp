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

#include <cobs_serial/data_utils.hpp>

#include <algorithm>
#include <cctype>

namespace cobs_serial::data_utils
{
constexpr std::array<char, 16> vChars = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

std::array<uint8_t, 2> from_int16(const int16_t value)
{
  // The result of this conversion does not depend on the endianness of the system.
  std::array<uint8_t, 2> bytes;
  bytes[0] = static_cast<uint8_t>(value >> 8) & 0xFF;  // MSB
  bytes[1] = static_cast<uint8_t>(value & 0xFF);       // LSB
  return bytes;
}

int16_t to_int16(const std::array<uint8_t, 2>& bytes)
{
  // The result of this conversion does not depend on the endianness of the system.
  const int16_t value = static_cast<int16_t>((bytes[0] << 8) + bytes[1]);
  return value;
}

std::array<uint8_t, 4> from_int32(const int32_t value)
{
  // The result of this conversion does not depend on the endianness of the system.
  std::array<uint8_t, 4> bytes;
  bytes[0] = static_cast<uint8_t>((value >> 24) & 0xFF);  // MSB
  bytes[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
  bytes[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
  bytes[3] = static_cast<uint8_t>(value & 0xFF);  // LSB
  return bytes;
}

int32_t to_int32(const std::array<uint8_t, 4>& bytes)
{
  // The result of this conversion does not depend on the endianness of the system.
  const int32_t value = (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
  return value;
}

std::array<uint8_t, 4> from_float(const float value)
{
  // The result of this conversion depends on the endianness of the system.
  const auto value_p = reinterpret_cast<const uint8_t*>(&value);
  std::array<uint8_t, 4> bytes;
  bytes[0] = value_p[3];
  bytes[1] = value_p[2];
  bytes[2] = value_p[1];
  bytes[3] = value_p[0];
  return bytes;
}

float to_float(const std::array<uint8_t, 4>& bytes)
{
  // The result of this conversion depends on the endianness of the system.
  float value;
  const auto value_p = reinterpret_cast<uint8_t*>(&value);
  value_p[0] = bytes[3];
  value_p[1] = bytes[2];
  value_p[2] = bytes[1];
  value_p[3] = bytes[0];
  return value;
}

std::string to_hex(const std::vector<uint8_t>& bytes)
{
  std::string hex;
  for (auto it = std::begin(bytes); it != std::end(bytes); ++it)
  {
    if (it != bytes.begin())
    {
      hex += " ";
    }
    hex += "";
    uint8_t ch = *it;
    hex += vChars[((ch >> 4) & 0xF)];
    hex += vChars[(ch & 0xF)];
  }

  return hex;
}

std::string to_hex(const std::vector<uint16_t>& bytes)
{
  std::string hex;
  for (auto it = std::begin(bytes); it != std::end(bytes); ++it)
  {
    if (it != bytes.begin())
    {
      hex += " ";
    }
    hex += "";
    uint16_t ch = *it;
    hex += vChars[((ch >> 12) & 0xF)];
    hex += vChars[((ch >> 8) & 0xF)];
    hex += vChars[((ch >> 4) & 0xF)];
    hex += vChars[(ch & 0xF)];
  }

  return hex;
}

std::string to_lower(const std::string& str)
{
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
  return result;
}

}  // namespace cobs_serial::data_utils
