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

#include <gmock/gmock.h>

#include <string>

#include <framed_serial/serial.hpp>

namespace framed_serial::test
{
class MockSerial : public framed_serial::Serial
{
public:
  MOCK_METHOD(void, open, (), (override));
  MOCK_METHOD(bool, is_open, (), (override, const));
  MOCK_METHOD(void, close, (), (override));
  MOCK_METHOD(std::size_t, read, (uint8_t * buffer, size_t size), (override));
  MOCK_METHOD(std::size_t, write, (const uint8_t* buffer, size_t size), (override));
  MOCK_METHOD(void, set_port, (const std::string& port), (override));
  MOCK_METHOD(std::string, get_port, (), (override, const));
  MOCK_METHOD(void, set_timeout, (std::chrono::duration<double> timeout), (override));
  MOCK_METHOD(std::chrono::duration<double>, get_timeout, (), (override, const));
  MOCK_METHOD(void, set_baudrate, (uint32_t baudrate), (override));
  MOCK_METHOD(uint32_t, get_baudrate, (), (override, const));
};
}  // namespace framed_serial::test
