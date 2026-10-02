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

#include <cstdint>
#include <vector>

/**
 * This utility provides methods to compute CRC-16 (Cyclic Redundancy Check) on
 * a sequence of bytes.
 * It implements an algorithm called CCITT CRC-16 (Kermit).
 *
 * A complete catalog of parametrized CRC algorithms with 16 bits is available at
 * http://reveng.sourceforge.net/crc-catalogue/16.htm
 *
 * A CRC online calculator useful for testing is available at
 * https://crccalc.com
 *
 * There are other online website to test a CRC but it seems the one above is
 * the only one to return the CRC in the correct little-endian order.
 * http://www.lammertbies.nl/comm/info/crc-calculation.html
 *
 * For a sample implementation of different CRC-16 see
 * http://www.lammertbies.nl/comm/software/
 */
namespace cobs_serial
{
/**
 * This method updates the given CRC adding a new byte to the original
 * sequence of bytes where the given CRC was computed.
 * The CCITT CRC-16 (Kermit) requires to pass an initial CRC equal to 0x0000
 * the first time the method is invoked.
 * @param crc The CRC value to update.
 * @param ch The byte to be added to the original sequence of bytes where
 *     the given CRC was computed.
 * @return The new CRC computed on the full sequence of bytes.
 */
[[nodiscard]] uint16_t crc_ccitt_byte(uint16_t crc, uint8_t ch);

/**
 * This method calculates the CRC on the given sequence bytes and length
 * stored in little-endian order.
 * @param bytes The sequence of bytes to calculate the CRC.
 * @return The CRC calculated on the given sequence of bytes.
 */
[[nodiscard]] uint16_t crc_ccitt(const std::vector<uint8_t>& buffer);

}  // namespace cobs_serial
