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
#include <framed_serial/crc_utils.hpp>

namespace framed_serial::test
{
TEST(TestCrcUtils, calculate_crc)
{
  ASSERT_EQ(framed_serial::crc_ccitt({ 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6 }), 0x3B07);
  ASSERT_EQ(framed_serial::crc_ccitt({ 0xE2, 0x12, 0xF1, 0xFF, 0x00, 0xD2 }), 0x7071);
  ASSERT_EQ(framed_serial::crc_ccitt({ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }), 0xFBE9);
  ASSERT_EQ(framed_serial::crc_ccitt({ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }), 0x0000);
  ASSERT_EQ(framed_serial::crc_ccitt({ 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 }), 0x2189);
  ASSERT_EQ(framed_serial::crc_ccitt({ 0x80, 0x00, 0x00, 0x03 }), 0x1ff5);
}
}  // namespace framed_serial::test
