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

#include <gtest/gtest.h>
#include <cobs_serial/data_utils.hpp>

namespace cobs_serial::test
{
TEST(TestDataUtils, to_hex)
{
  std::vector<uint8_t> bytes = { 0x7E, 0x00, 0x70, 0x00, 0x00, 0x00, 0x4E, 0x20, 0x75, 0x38, 0x7E };
  std::string hex = cobs_serial::data_utils::to_hex(bytes);
  ASSERT_EQ(hex, "7E 00 70 00 00 00 4E 20 75 38 7E");
}

TEST(TestDataUtils, from_float)
{
  // See https://www.h-schmidt.net/FloatConverter/IEEE754.html
  // Convert 4.2 into a sequence of bytes.
  auto bytes = cobs_serial::data_utils::from_float(4.2f);
  std::vector<uint8_t> bytes_v(bytes.begin(), bytes.end());
  auto hex = cobs_serial::data_utils::to_hex(bytes_v);
  ASSERT_EQ(hex, "40 86 66 66");
}

TEST(TestDataUtils, to_float)
{
  // See https://www.h-schmidt.net/FloatConverter/IEEE754.html
  // Check if the given sequence of bytes equals to the float 4.2.
  std::array<uint8_t, 4> bytes = { 0x40, 0x86, 0x66, 0x66 };
  auto value = cobs_serial::data_utils::to_float(bytes);
  ASSERT_FLOAT_EQ(value, 4.2f);
}

TEST(TestDataUtils, from_int32)
{
  auto bytes = cobs_serial::data_utils::from_int32(-1582119980);
  std::vector<uint8_t> bytes_v(bytes.begin(), bytes.end());
  auto hex = cobs_serial::data_utils::to_hex(bytes_v);
  ASSERT_EQ(hex, "A1 B2 C3 D4");
}

TEST(TestDataUtils, to_int32)
{
  std::array<uint8_t, 4> bytes = { 0xA1, 0xB2, 0xC3, 0xD4 };
  auto value = cobs_serial::data_utils::to_int32(bytes);
  ASSERT_EQ(value, -1582119980);
}

TEST(TestDataUtils, from_int16)
{
  auto bytes = cobs_serial::data_utils::from_int16(-3937);
  std::vector<uint8_t> bytes_v(bytes.begin(), bytes.end());
  auto hex = cobs_serial::data_utils::to_hex(bytes_v);
  ASSERT_EQ(hex, "F0 9F");
}

TEST(TestDataUtils, to_int16)
{
  std::array<uint8_t, 2> bytes = { 0xF0, 0x9F };
  auto value = cobs_serial::data_utils::to_int16(bytes);
  ASSERT_EQ(value, -3937);
}

TEST(TestDataUtils, to_lower)
{
  const char* str1 = "HeLlO";
  ASSERT_EQ(data_utils::to_lower(str1), "hello");

  std::string str2{ "HeLlO" };
  ASSERT_EQ(data_utils::to_lower(str2), "hello");
}
}  // namespace cobs_serial::test
