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

#include <exception>
#include <sstream>
#include <string>

namespace framed_serial
{
class SerialException : public std::exception
{
  std::string what_;

public:
  explicit SerialException(const std::string& description)
  {
    std::stringstream ss;
    ss << "SerialException: " << description << ".";
    what_ = ss.str();
  }

  SerialException(const SerialException& other) : what_(other.what_)
  {
  }

  ~SerialException() override = default;

  // Disable copy constructors
  SerialException& operator=(const SerialException&) = delete;

  [[nodiscard]] const char* what() const throw() override
  {
    return what_.c_str();
  }
};
}  // namespace framed_serial
