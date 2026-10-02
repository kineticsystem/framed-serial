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

#include <cobs_serial/cobs_serial_factory.hpp>
#include <cobs_serial/default_cobs_serial_factory.hpp>
#include <hardware_interface/hardware_info.hpp>

namespace cobs_serial
{
/**
 * This class is used to create a default driver to interact with the hardware.
 */
class DefaultCobsSerialFactory : public CobsSerialFactory
{
public:
  DefaultCobsSerialFactory() = default;

  /**
   * @brief Create a cobs serial interface.
   * @param info The hardware information.
   * @return A sarial interface to communicate with the hardware.
   */
  std::unique_ptr<CobsSerial> create(const hardware_interface::HardwareInfo& info) const;
};
}  // namespace cobs_serial
