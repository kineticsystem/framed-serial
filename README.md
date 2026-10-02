# Framed Serial

## Table of Contents <!-- omit in toc -->

- [Introduction](#introduction)
- [The Frame](#the-frame)
- [Prerequisites](#prerequisites)
- [Install Framed Serial](#install-framed-serial)
  - [Check out the Git Repositories](#check-out-the-git-repositories)
  - [Build the Project](#build-the-project)
- [Using the Library](#using-the-library)
  - [Open a Connection](#open-a-connection)
  - [Test Without a Serial Port](#test-without-a-serial-port)
- [Tests](#tests)
- [Limitations](#limitations)
- [License](#license)

## Introduction

Framed Serial is a C++ library and ROS2 package that sends and receives messages over a serial port, e.g. to a microcontroller on USB. It cuts the stream of bytes into frames, so that each call to `write()` arrives as one message, and checks each frame with a CRC, so that a damaged message is detected instead of read.

- Write a message as one frame, with its delimiters, escaped bytes and CRC.
- Read the next frame, check its CRC and return its data, or throw an exception on a timeout or a damaged frame.
- Replace the serial port with a mock in unit tests, through the `Serial` and `FramedSerial` interfaces.

The frames are simple enough for a microcontroller to read and write with a few lines of code and a small buffer, so the library suits a host that talks to an Arduino, a Teensy or a similar board.

## The Frame

A frame is the data of one message, its CRC, and a delimiter at each end:

| Part | Size | Content |
|---|---|---|
| start delimiter | 1 byte | `0x7E` |
| data | 1 byte or more | the message, escaped |
| CRC | 2 bytes | the CRC-16/KERMIT of the data, low byte first, escaped |
| end delimiter | 1 byte | `0x7E` |

A byte equal to `0x7E` or `0x7D` inside the data or the CRC is escaped: it is sent as `0x7D` followed by the byte XOR `0x20`. So `0x7E` becomes `0x7D 0x5E` and `0x7D` becomes `0x7D 0x5D`, and a `0x7E` on the wire is always a delimiter. A reader that joins the stream in the middle of a frame waits for the next delimiter and loses nothing else.

The CRC is CRC-16/KERMIT: polynomial `0x1021` reflected, initial value `0x0000`. Its check value, the CRC of the ASCII bytes `123456789`, is `0x2189`. Sent low byte first, the CRC computed over the data and the CRC together is `0x0000`, which is how a reader checks a frame.

Numbers inside the data are not the library's concern. The projects that use it send them most significant byte first, and [`data_utils.hpp`](include/framed_serial/data_utils.hpp) converts integers and floats in that order.

## Prerequisites

To build Framed Serial, we need a computer with Ubuntu 24.04 and ROS2 Jazzy. Please refer to the document [Install ROS2 Jazzy on Ubuntu](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html).

The library depends on two packages:

- [`serial`](https://github.com/kineticsystem/serial), branch `ros2`, a cross-platform serial port library.
- `rclcpp`, for logging. It comes with ROS2.

## Install Framed Serial

### Check out the Git Repositories

Framed Serial is a package of a colcon workspace. Check it out, together with the `serial` library, in the `src` folder of the workspace:

```
cd ~/ws/src
git clone https://github.com/kineticsystem/framed-serial.git
git clone --branch ros2 https://github.com/kineticsystem/serial.git
```

In a project that keeps its dependencies in git, add them as submodules instead:

```
git submodule add https://github.com/kineticsystem/framed-serial.git modules/framed-serial
git submodule add --branch ros2 https://github.com/kineticsystem/serial.git modules/serial
```

### Build the Project

From the root of the workspace, install the dependencies:

```
rosdep install --ignore-src --from-paths . -y -r
```

Build the packages:

```
colcon build --packages-up-to framed_serial
```

Execute the tests:

```
colcon test --packages-select framed_serial
colcon test-result --verbose
```

## Using the Library

A ROS2 package that uses the library depends on it in its `package.xml`:

```xml
<depend>framed_serial</depend>
```

and links it in its `CMakeLists.txt`:

```cmake
find_package(framed_serial REQUIRED)
ament_target_dependencies(my_driver framed_serial)
```

### Open a Connection

`DefaultSerial` is the serial port and `DefaultFramedSerial` the frames on top of it:

```cpp
#include <framed_serial/default_framed_serial.hpp>
#include <framed_serial/default_serial.hpp>

auto serial = std::make_unique<framed_serial::DefaultSerial>();
serial->set_port("/dev/ttyUSB0");
serial->set_baudrate(9600);
serial->set_timeout(std::chrono::duration<double>{ 0.2 });

framed_serial::DefaultFramedSerial connection{ std::move(serial) };
connection.open();

connection.write({ 0x76 });                     // one frame
std::vector<uint8_t> answer = connection.read();  // the data of the next frame
```

`read()` blocks until a whole frame has arrived. It throws `framed_serial::SerialException` when:

- **no byte arrives within the timeout**, `timeout`. The timeout applies to each byte, not to the whole frame.
- **the first byte is not a delimiter**, `start delimiter missing`. This happens when the reader starts in the middle of a frame, e.g. right after the device resets.
- **the frame is shorter than three bytes**, `incorrect frame length`: one byte of data and the two bytes of the CRC.
- **the CRC does not match**, `CRC error`.

The caller decides what to do: a driver usually retries the request a few times before it reports the device as lost.

### Test Without a Serial Port

A driver that takes a `std::unique_ptr<framed_serial::FramedSerial>` can be tested with a mock of that interface, which returns the frames the test chooses and records the frames the driver writes. The library tests `DefaultFramedSerial` itself against a mock of `Serial`, in [`tests/mock/mock_serial.hpp`](tests/mock/mock_serial.hpp).

## Tests

| Test | What it covers |
|---|---|
| `test_crc_utils` | the CRC-16/KERMIT of a known sequence |
| `test_data_utils` | the conversion of integers and floats to bytes, most significant byte first, and back, and the hex and lowercase helpers |
| `test_default_framed_serial` | the frames written and read: delimiters, escaped data, an escaped CRC, a failed write, and the exceptions on a timeout, a missing delimiter, a short frame and a bad CRC |

## Limitations

**A frame is limited to 256 bytes.** `DefaultFramedSerial` assembles a frame in buffers of 256 bytes, and a byte that does not fit is dropped without an error:

- **Writing:** the buffer holds the frame as it goes on the wire, with its delimiters, escaped bytes and CRC. Data of up to 125 bytes always fits, even when every byte has to be escaped; data of up to 250 bytes fits when none has to. A longer frame reaches the device cut short, and the device drops it on its CRC.
- **Reading:** the buffer holds the data and the CRC, unescaped, so data of up to 254 bytes fits. A longer frame is worse: the CRC is computed over every byte received, so it still matches, and `read()` returns the data cut short, without its last bytes, and without an error.

Keep the messages of a protocol well under these sizes, or check their length before sending them.

**One request at a time.** The library reads whatever frame comes next. It does not match an answer to its request, so a protocol on top of it either waits for each answer before the next request, or carries its own sequence numbers.

## License

Framed Serial is released under the [MIT License](LICENSE).
