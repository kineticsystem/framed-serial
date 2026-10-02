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

#include <vector>

namespace framed_serial
{

enum class BufferPosition
{
  Head,
  Tail
};

/**
 * This template implements a circular buffer. Items can be added and removed
 * to and from both sides of the buffer.
 */
template <typename T>
class Buffer
{
public:
  explicit Buffer(std::size_t capacity);

  // Return how much data is in the buffer.
  [[nodiscard]] std::size_t size() const;

  // Insert an item at the given position.
  void add(T in, BufferPosition position);

  // Remove an item from the given position.
  // Removing an item from an empty buffer leads to undefined behavior.
  T remove(BufferPosition position);

  // Reset the buffer.
  void clear();

private:
  // The buffer data structure.
  std::vector<T> data_;

  // The position of the first element of the buffer.
  std::size_t position_ = 0;

  // The buffer capacity.
  std::size_t capacity_ = 0;

  // This records how much data is in the buffer: it is always greater
  // than zero and less than or equal to the capacity.
  // "position_ + size_ - 1" is the position of the last element of the
  // buffer.
  std::size_t size_ = 0;
};

template <typename T>
Buffer<T>::Buffer(std::size_t capacity) : capacity_{ capacity }
{
  data_.reserve(capacity);
}

template <typename T>
std::size_t Buffer<T>::size() const
{
  return size_;
}

template <typename T>
void Buffer<T>::clear()
{
  position_ = 0;
  size_ = 0;
}

template <typename T>
void Buffer<T>::add(T in, BufferPosition position)
{
  // Please note that there is no exception thrown in integer arithmetic
  // overflow or underflow.

  if (size_ < capacity_)
  {
    if (position == BufferPosition::Tail)
    {
      // Add an item to the end of the buffer.
      data_[(position_ + size_) % capacity_] = in;
    }
    else
    {
      // Add an item to the front of the buffer.
      if (position_ == 0)
      {
        position_ = capacity_ - 1;
      }
      else
      {
        position_--;
      }
      data_[position_] = in;
    }
    size_++;
  }
}

template <typename T>
T Buffer<T>::remove(BufferPosition position)
{
  // Please note that there is no exception thrown in integer arithmetic
  // overflow or underflow.

  T out;
  if (size_ > 0)
  {
    if (position == BufferPosition::Tail)
    {
      // Remove an item from the end of the buffer.
      out = data_[(position_ + size_ - 1) % capacity_];
    }
    else
    {
      // Remove an item from the front of the buffer.
      out = data_[position_];
      position_ = (position_ + 1) % capacity_;
    }
    size_--;
  }
  return out;
}
}  // namespace framed_serial
