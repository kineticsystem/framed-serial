# Framed Serial

[![CI](https://github.com/kineticsystem/framed-serial/actions/workflows/industrial_ci.yml/badge.svg)](https://github.com/kineticsystem/framed-serial/actions/workflows/industrial_ci.yml)
[![Format](https://github.com/kineticsystem/framed-serial/actions/workflows/ci-format.yml/badge.svg)](https://github.com/kineticsystem/framed-serial/actions/workflows/ci-format.yml)
[![Linters](https://github.com/kineticsystem/framed-serial/actions/workflows/ci-ros-lint.yml/badge.svg)](https://github.com/kineticsystem/framed-serial/actions/workflows/ci-ros-lint.yml)

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
- [Continuous Integration](#continuous-integration)
- [Limitations](#limitations)
- [License](#license)

## Introduction

Framed Serial is a C++ library and ROS2 package that sends and receives messages over a serial port, e.g. to a microcontroller on USB. It cuts the stream of bytes into frames, so that each call to `write()` arrives as one message, and checks each frame with a CRC, so that a damaged message is detected instead of read.

- Write a message as one frame, with its delimiters, escaped bytes and CRC.
- Read the next frame, check its CRC and return its data, or throw an exception on a timeout or a damaged frame.
- Replace the serial port with a mock in unit tests, through the `Serial` and `FramedSerial` interfaces.

The frames are simple enough for a microcontroller to read and write with a few lines of code and a small buffer, so the library suits a host that talks to an Arduino, a Teensy or a similar board.

How the library is built, and where to start a change, is in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md). Where the code falls short of what it is meant to do, with the measurements, is in [docs/Review.md](docs/Review.md).

## The Frame

A frame is the data of one message, its CRC, and a delimiter at each end:

| Part | Size | Content |
|---|---|---|
| start delimiter | 1 byte | `0x7E` |
| data | 1 byte or more | the message, escaped |
| CRC | 2 bytes | the CRC-16/KERMIT of the data, low byte first, escaped |
| end delimiter | 1 byte | `0x7E` |

A byte equal to `0x7E` or `0x7D` inside the data or the CRC is escaped: it is sent as `0x7D` followed by the byte XOR `0x20`. So `0x7E` becomes `0x7D 0x5E` and `0x7D` becomes `0x7D 0x5D`, and a `0x7E` on the wire is a delimiter. The one exception is a damaged frame that leaves a `0x7E` right after a `0x7D`: the reader takes it as an escaped byte, not as a delimiter.

The reader of this library does not look for the next delimiter. When it joins the stream in the middle of a frame, `read()` throws, and the caller puts it back in step: see [Open a Connection](#open-a-connection).

The CRC is CRC-16/KERMIT: polynomial `0x1021` reflected, initial value `0x0000`. Its check value, the CRC of the ASCII bytes `123456789`, is `0x2189`. Sent low byte first, the CRC computed over the data and the CRC together is `0x0000`, which is how a reader checks a frame.

Numbers inside the data are not the library's concern. The projects that use it send them most significant byte first, and [`data_utils.hpp`](include/framed_serial/data_utils.hpp) converts 16-bit and 32-bit integers and 32-bit floats in that order, the floats on a little-endian host only. It also declares `from_int8()` and `to_int8()`, which are defined nowhere: a call to them does not link.

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

A project that depends on Framed Serial lists both repositories in a `.repos` file, each pinned to a commit or a tag, and imports them with `vcs`, e.g. into a `modules` folder that git ignores:

```yaml
repositories:
  serial:
    type: git
    url: https://github.com/kineticsystem/serial.git
    version: <commit>
  framed-serial:
    type: git
    url: https://github.com/kineticsystem/framed-serial.git
    version: <commit>
```

```
mkdir -p modules
vcs import --input <project>.repos modules
```

Put the commits of `serial` and `framed-serial` in place of `<commit>`, and the name of the file in place of `<project>.repos`. A workspace that brings together several projects using Framed Serial lists it once, in its own `.repos` file: colcon refuses two packages of the same name.

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

The exceptions of the port are not caught, and they are not of that type: `serial::IOException`, `serial::SerialException` and `serial::PortNotOpenedException`, e.g. for a port that does not exist, a device that is unplugged or a port that is not open, and `std::invalid_argument` from `open()` when no port is set. They pass through `read()`, `write()` and `open()` as they are, and have only `std::exception` in common with `framed_serial::SerialException`: a caller that must survive a lost device catches `std::exception`.

The caller decides what to do: a driver usually retries the request a few times before it reports the device as lost. A retry alone is not always enough. `read()` stops where it failed and does not look for the next frame, so after `start delimiter missing`, `incorrect frame length` or a `timeout` in the middle of a frame, the next calls start inside a frame and fail too. Before it sends the request again, the caller empties the port, by calling `read()` until it throws `timeout` with nothing left to come, or closes the port and opens it again. [docs/Review.md](docs/Review.md) measures each case.

### Test Without a Serial Port

A driver that takes a `std::unique_ptr<framed_serial::FramedSerial>` can be tested with a mock of that interface, which returns the frames the test chooses and records the frames the driver writes. The library tests `DefaultFramedSerial` itself against a mock of `Serial`, in [`tests/mock/mock_serial.hpp`](tests/mock/mock_serial.hpp).

## Tests

| Test | What it covers |
|---|---|
| `test_crc_utils` | the CRC-16/KERMIT of a known sequence |
| `test_data_utils` | the conversion of integers and floats to bytes, most significant byte first, and back, and the hex and lowercase helpers |
| `test_default_framed_serial` | the frames written and read: delimiters, escaped data, an escaped CRC, a failed write, and the exceptions on a timeout, a missing delimiter, a short frame and a bad CRC |

## Continuous Integration

Three GitHub Actions workflows run on every push and pull request, as in the StepIt projects that use the library:

| Workflow | What it checks |
|---|---|
| [`industrial_ci.yml`](.github/workflows/industrial_ci.yml) | Builds the package and runs its tests with [Industrial CI](https://github.com/ros-industrial/industrial_ci), against the main and the testing ROS repositories. |
| [`ci-format.yml`](.github/workflows/ci-format.yml) | The C++ code against [`.clang-format`](.clang-format), with the clang-format of Ubuntu 24.04. |
| [`ci-ros-lint.yml`](.github/workflows/ci-ros-lint.yml) | The ament linters: copyright, lint_cmake and cpplint. |

Industrial CI builds `serial` first, from [`framed-serial.repos`](framed-serial.repos), at the commit StepIt Motors and StepIt Freezer build it with: move it there when they move. `ament_copyright` needs [`CONTRIBUTING.md`](CONTRIBUTING.md) next to `LICENSE`, as the package is the root of the repository.

## Limitations

**A frame is limited to 256 bytes.** `DefaultFramedSerial` assembles a frame in buffers of 256 bytes, and a byte that does not fit is dropped without an error:

- **Writing:** the buffer holds the frame as it goes on the wire, with its delimiters, escaped bytes and CRC. Data of up to 125 bytes always fits, even when every byte has to be escaped; data of up to 250 bytes fits when none has to. A longer frame leaves cut short, without its end delimiter, and `write()` returns normally. The device ends it on the start delimiter of the next frame and drops it on its CRC; a device that then waits for a new start delimiter loses that next frame too.
- **Reading:** the buffer holds the data and the CRC, unescaped, so data of up to 254 bytes fits. A longer frame is worse: the CRC is computed over every byte received, so it still matches, and `read()` returns the data cut short, without its last bytes, and without an error.

Keep the messages of a protocol well under these sizes, or check their length before sending them. A device with a smaller buffer sets a lower limit: the sizes above are those of this library only.

**No empty message.** A frame holds at least one byte of data, but `write()` does not check it: called with no bytes, it sends `7E 00 00 7E`, which `read()` refuses with `incorrect frame length`.

**One request at a time.** The library reads whatever frame comes next. It does not match an answer to its request, so a protocol on top of it either waits for each answer before the next request, or carries its own sequence numbers.

## License

Framed Serial is released under the [MIT License](LICENSE).
