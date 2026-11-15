# Framed Serial Review

## Table of Contents <!-- omit in toc -->

- [Scope and Method](#scope-and-method)
- [Verdict](#verdict)
- [Structure and Boundaries](#structure-and-boundaries)
- [Interfaces and Abstractions](#interfaces-and-abstractions)
- [The Exception Contract](#the-exception-contract)
- [The Reader](#the-reader)
- [The Writer](#the-writer)
- [How the Two Users Call It](#how-the-two-users-call-it)
- [SOLID](#solid)
- [Coupling and Duplication](#coupling-and-duplication)
- [The Build](#the-build)
- [Testability and Tests](#testability-and-tests)
- [Implementation Issues](#implementation-issues)
- [Suspicions a Probe Disproved](#suspicions-a-probe-disproved)
- [What the Documents Say and the Code Does Not](#what-the-documents-say-and-the-code-does-not)
- [Recommendations](#recommendations)

Reviewed on 2026-11-15, at commit `d386c91`. It judges the design that [docs/ARCHITECTURE.md](ARCHITECTURE.md) describes, and does not repeat it.

## Scope and Method

This review covers the whole package: the 8 headers of [`include/framed_serial`](../include/framed_serial) and the 4 sources of [`src`](../src), 1219 lines; [`CMakeLists.txt`](../CMakeLists.txt) and [`package.xml`](../package.xml); and the 3 test programs of [`tests`](../tests) with their mock, 659 lines. We read every file, raw.

We also read the neighbours, as far as they touch the library: the other end of the wire, `SerialPort.cpp`, `Buffer.h` and `CrcUtils.cpp` of the firmware of [StepIt Motors](https://github.com/kineticsystem/stepit-motors) (`src/stepit_mcu`, commit `dc269f8`) and of [StepIt Freezer](https://github.com/kineticsystem/stepit-freezer) (`src/freezer_mcu`, commit `f4aa88e`); the two callers, `stepit_driver` and `freezer_driver`; and `unix.cc` of the [`serial`](https://github.com/kineticsystem/serial) library (commit `d8d1606`), which is third-party and not reviewed.

Each suspicion was put to a throwaway test before it was reported. The probes were 21 GoogleTest cases in a package of their own, built with the library into a scratch folder inside the container the two projects are built in (Ubuntu 24.04, GCC 13.3, ROS2 Jazzy), with the repository mounted read-only, then deleted. They drive the real `DefaultFramedSerial` on a fake `Serial` that is a queue of bytes; where the device matters, the fake feeds a line-by-line transcription of the reading half of the firmware's `SerialPort::update()`, and answers every good request with a 14-byte reply that carries the number of its request. Three probes run the real `DefaultSerial` on a pseudo-terminal. No board was touched: what the firmware does is from reading, or from the transcription. Each probe states what a caller would expect, so a finding marked **verified** failed its probe; 17 of the 21 failed. The others are **from reading**. The library was also built three more times, with `-D_GLIBCXX_ASSERTIONS`, with `-fsanitize=address,undefined`, and with both sanitizers and `-D_GLIBCXX_SANITIZE_VECTOR`, and its own tests run on each.

The whole suite, once: `colcon test --packages-select framed_serial` runs 3 test programs, 19 cases, with no failure and no skip, in 0.16 s. It runs no linter: see [The Build](#the-build).

## Verdict

The structure is right and small: two interfaces, one above the frames and one below, no cycle, one file that knows the `serial` library, and a frame format, byte stuffing with a CRC-16, that is a sound choice for a microcontroller. A driver built on `FramedSerial` is easy to test, and both users do it.

The implementation behind the interfaces does not keep the one idea the architecture states, **a call is a whole message or an exception**. A call can return part of a message, and send part of one, without an exception (issue 5); and after one exception the next calls are not whole messages either. That is the main weakness: **the reader cannot recover from a damaged stream**. A reply that loses its first byte, gains a stray delimiter or arrives in two halves leaves the reader out of step for good: 0 of the next 250 requests are answered (issue 1, verified). A reply that is late, or that follows one stray byte, leaves the caller one reply behind for good, each `read()` returning the answer to the request before with a good CRC (issue 2, verified). The interface offers no way to drop what waits in the port, so the only recovery is to close the port, or to call `read()` until it times out with nothing left, hundreds of calls. The firmware at the other end, written for the same frames, does resynchronise; the host's reader is the weaker copy.

The second weakness is latent: `Buffer` writes into a vector that has no elements (issue 3, verified). It works with the compiler and the flags of today, and aborts on the first `write()` as soon as the standard library checks its bounds.

**No finding is a safety issue for a robot driven through the library**, for a reason outside it: the firmware of StepIt Motors stops every motor 1 s after the last good frame, and each failure found here ends with a host that goes silent or a caller that reports the device as lost. The findings cost availability: a run ends on one damaged byte. The closest to a safety issue is issue 7, a stop command dropped by the device because the write before it failed.

Little guards any of this: at the commit reviewed the repository had no CI, `colcon test` registers no linter, and its 19 tests make one call each on a fresh object. `main` has since gained three GitHub workflows, which build, test, lint and check the format; they run the same 19 tests, so the findings stand.

## Structure and Boundaries

| Rule of the architecture | Verdict | Evidence |
|---|---|---|
| Only `default_serial.hpp` and `.cpp` include `serial/serial.h` | Kept | 2 includes, in those 2 files. |
| `DefaultFramedSerial` never reads the content of a message | Kept | `read()` and `write()` of [`default_framed_serial.cpp`](../src/default_framed_serial.cpp) touch only delimiters, escapes and the CRC. |
| The helpers include no other header of the library | Kept | `buffer.hpp`, `crc_utils.hpp` and `data_utils.hpp` include the standard library only. |
| The interfaces name no frame and no port library | Kept in the code, broken in the comments | `serial.hpp` documents `@throw serial::PortNotOpenedException`, `serial::SerialException` and `serial::IOException` (lines 57, 58, 68 to 70) and refers to `SerialInterface::setPort`, a name that no longer exists. |
| No cycle | Kept | `default_framed_serial` → `framed_serial`, `serial`, `buffer`, `crc_utils`, `data_utils`, `serial_exception`; `default_serial` → `serial`. One direction. |
| `rclcpp` only for one log line | Kept, at a price | 1 include, `rclcpp/logging.hpp`; the price is in [The Build](#the-build). |

One thing sits in the wrong place: **the library holds helpers that are not about frames**. `data_utils` has 13 functions; `DefaultFramedSerial` uses 1, `to_hex()`. Of the others, the two users call 3 (`from_float()`, `to_float()`, `to_lower()`); 2 are declared and never defined, and 7 (`from_int16()`, `to_int16()`, `from_int32()`, `to_int32()`, `stoui16()`, `stoui32()`, `to_hex()` of 16-bit values) have no caller in either project. StepIt Freezer wrote its own `to_uint16()` and `to_uint32()` rather than use them. `to_lower()` and the two `stoui` are about a caller's settings, not about bytes on a wire.

## Interfaces and Abstractions

| Interface | Where | Judgement |
|---|---|---|
| `FramedSerial` | [`framed_serial.hpp`](../include/framed_serial/framed_serial.hpp) | The right seam, 5 methods, mocked by both users in 16 lines. It lacks the one call a request-and-reply protocol needs after an error, to drop what waits in the port. It has no timeout either: the caller sets one on the `Serial` before handing it over, and cannot change it afterwards. `is_open()` is not `const`, while `Serial::is_open()` is. |
| `Serial` | [`serial.hpp`](../include/framed_serial/serial.hpp) | 11 pure methods, of which `DefaultFramedSerial` calls 5 (`open`, `is_open`, `close`, `read`, `write`). The 6 setters and getters of port, baud rate and timeout are configuration of the real port, which a fake does not have; every fake must still implement them. `read()` returning 0 means a timeout and `write()` returning 0 an error: two meanings of 0 that only the comments of the callers tell. No `flush_input()`, though `serial::Serial::flushInput()` exists underneath. |
| `DefaultFramedSerial` | [`default_framed_serial.hpp`](../include/framed_serial/default_framed_serial.hpp) | Owns its `Serial`; a null pointer is accepted by the constructor and dereferenced by `open()`. `state_` is a member, yet `read()` sets it at each call and nothing else reads it: it is a local variable. `add_escaped_byte()` is a free function with external linkage, exported by the shared library though no header declares it. |
| `DefaultSerial` | [`default_serial.hpp`](../include/framed_serial/default_serial.hpp) | A thin adapter. Its public header includes `serial/serial.h` and then forward-declares `serial::Serial`: one of the two is not needed, and with an out-of-line destructor the header would not expose the third-party library at all. `read()` has a default argument, `size = 1`, that the interface does not have (line 52). |
| `SerialException` | [`serial_exception.hpp`](../include/framed_serial/serial_exception.hpp) | One type for four different failures, told apart only by the text of `what()`. See [The Exception Contract](#the-exception-contract). Its copy assignment is deleted under the comment "Disable copy constructors", and `what()` is declared `throw()`, removed from the language in C++20, which GCC still accepts. |
| `Buffer<T>` | [`buffer.hpp`](../include/framed_serial/buffer.hpp) | A circular buffer with two ends, of which the library uses a queue: add at the tail, remove at the head, and twice remove at the tail. A `std::vector` would do. Issues 3, 11 and 12. |
| `crc_ccitt_byte()`, `crc_ccitt()` | [`crc_utils.hpp`](../include/framed_serial/crc_utils.hpp) | Correct: the check value `0x2189` of CRC-16/KERMIT is tested. The header's comment says the bytes are "stored in little-endian order", which means nothing for a sequence of bytes. |
| `data_utils` | [`data_utils.hpp`](../include/framed_serial/data_utils.hpp) | `to_hex({ 0x7E })` does not compile: the call is ambiguous between the overloads for 8-bit and 16-bit values (verified, by the compiler). `stoui16()` and `stoui32()` return 0 for a text that is not a number. |
| The factories | none | The commit reviewed removed them; each user builds the two objects itself, in `DefaultSerialFactory` of `stepit_driver` and `create_driver()` of `freezer_node`. Right: the library should not know where a caller keeps its settings. |

## The Exception Contract

The headers promise one thing: `@throw framed_serial::SerialException`, on `read()` and `write()` of both `FramedSerial` and `DefaultFramedSerial`. The code throws from two unrelated families, verified with the real `DefaultSerial`:

| Call | What it throws |
|---|---|
| `read()` or `write()` on a port that is not open | `serial::PortNotOpenedException` |
| `open()` of a port that does not exist | `serial::IOException` |
| `open()` with no port set | `std::invalid_argument` |
| a device unplugged during a call | `serial::SerialException` or `serial::IOException`, from reading `unix.cc` |
| a timeout, a missing delimiter, a short frame, a bad CRC, a byte the port did not take | `framed_serial::SerialException` |

Two classes are named `SerialException`, one in each namespace, with only `std::exception` in common. The architecture document says so; the headers and the README do not. It costs nothing today: the 17 `catch` clauses of the two users all catch `std::exception`. It will cost the first caller that follows the header and catches `framed_serial::SerialException` to retry: an unplugged device then ends its process.

The second flaw is that the four failures of `read()` are one type. A caller that wants to retry after `timeout` but resynchronise after `start delimiter missing` must compare `what()` with a sentence. The frame that failed its CRC is not in the exception: it goes to the log.

## The Reader

`DefaultFramedSerial::read()` throws on the first thing it does not expect and leaves the port where it stopped: lines 94 and 108 of [`default_framed_serial.cpp`](../src/default_framed_serial.cpp). Nothing puts it back in step with the frames. The probes send one damaged reply, then count what happens to the requests that follow, each answered correctly by the device:

| Damage to one reply | Of the next 20 requests | Of the next 250 | Unread bytes after 250 |
|---|---|---|---|
| Its start delimiter is lost | 0 answered, 20 throw | 0 answered | 4258 |
| A stray `0x7E` before it | 0 answered, 20 throw | 0 answered | 4259 |
| Its second half comes after the timeout | 0 answered, 20 throw | 0 answered | 4255 |
| A stray `0x00` before it | 0 answered, 20 return the reply of the request before | 0 answered, 250 stale | 18 |
| The whole reply comes after the timeout | 0 answered, 20 stale | 0 answered, 250 stale | 18 |
| One byte of data is damaged | 20 answered | 250 answered | 0 |
| Its end delimiter is lost | 20 answered | 250 answered | 0 |

In the first three cases each `read()` consumes one or two bytes, 236 times `start delimiter missing` and 14 times `incorrect frame length`, while the device adds a reply of 18 bytes: the backlog grows and never ends. In the next two the frames are intact and one behind, so nothing ever throws. In each case the only way back that the interface offers worked: reading until a `timeout` with nothing left, about 335 calls after 20 requests and 4020 after 250, or closing the port.

The firmware's reader, fed the same kinds of damage, does better: in `Waiting` it skips every byte up to a delimiter, and a delimiter after fewer than 3 bytes starts a new frame. It has one flaw of its own, shared by both firmwares: after a frame of 3 bytes or more with a bad CRC it goes back to `Waiting`, so a delimiter that was in truth the start of the next frame is spent as an end, and that next frame is skipped. Verified with the transcription: see issue 7.

Three more properties of the reader, all verified:

- **A `read()` has no bound.** The timeout is per byte, and the loop has no limit on bytes. A frame of 18 wire bytes that trickles in at one byte every 40 ms, under a timeout of 50 ms, returns after 722 ms. A device that sends a start delimiter and then 1,000,000 bytes without another keeps one `read()` busy for 1,000,002 calls to the port, though a frame cannot hold more than 256.
- **After an escape, a delimiter is data.** `7E 11 7D` followed by a good frame is read as one frame with a bad CRC, and the good frame is gone. HDLC and PPP abort a frame on this sequence; the firmware has the same code.
- **Zeros are a valid message.** `7E 00 00 00 00 00 7E` returns three bytes `00 00 00`, since the CRC starts at 0 and a zero byte leaves it at 0. For the same reason any number of `0x00` in front of a message keeps its CRC: `0x1705` with none, one or three.

## The Writer

`write()` assembles a frame in a buffer of 256 bytes that drops what does not fit, and sends it one byte a call. Verified:

- **A frame that does not fit leaves cut, and `write()` returns normally.** Data of 252 bytes without an escape fit, 256 bytes on the wire; 253 put 256 bytes on the wire, the last one a byte of the CRC, with no end delimiter and no exception.
- **A message of no bytes is sent**, as `7E 00 00 7E`, a frame that the library's own reader refuses with `incorrect frame length` and that the firmware drops.
- **A write that fails halfway leaves half a frame on the wire.** `write()` throws and the next `write()` sends a new frame behind it. The start delimiter of that frame ends the half one at the device; with the firmware's reader, the new frame is then skipped (issue 7).
- **One byte a call costs little in software.** Through the real `DefaultSerial` on a pseudo-terminal, a frame of 27 wire bytes takes 50 µs to write, 1.8 µs a byte, and a frame of 70 wire bytes 33 µs to read. Per byte written, `DefaultSerial::write()` makes a `pselect()`, a `write()` and a `tcdrain()` (line 54 of [`default_serial.cpp`](../src/default_serial.cpp)). What a `tcdrain()` per byte costs on a real device depends on its driver, and was not measured here.

## How the Two Users Call It

Both hold a `std::unique_ptr<FramedSerial>` and do one thing with it: `write(request)` then `read()`, 5 times in `stepit_driver` and 6 in `freezer_driver`, with a per-byte timeout of 0.2 s. Neither drops unread input, neither checks that a reply belongs to its request beyond its length, and both catch `std::exception`.

`stepit_driver` calls the pair twice in each cycle of a 100 Hz control loop, a status query and a command. That thread is therefore exposed to everything above: a lost reply holds it for 0.2 s, 20 cycles, before the exception; a reply that trickles in holds it for longer, with no bound. Its largest message, a status of one byte and 13 per motor, stays far from the 254 bytes where a read is cut. On a `CRC error` the reader builds a string and logs it at the error level, and each `read()` and each exception allocate: none of it belongs in a realtime thread, though the loop of this robot tolerates it.

The frames in the tests are older than both protocols: they start with a "Request ID" byte that neither user sends.

## SOLID

| Principle | Verdict | Evidence |
|---|---|---|
| Single responsibility | Mostly kept | `DefaultFramedSerial` frames, `DefaultSerial` adapts. The package also carries number conversion and string helpers that no part of it uses: [Structure and Boundaries](#structure-and-boundaries). |
| Open/closed | Kept | Another transport is a class implementing `Serial`, another framing one implementing `FramedSerial`; nothing else changes. |
| Liskov substitution | Broken in the documented contract | `DefaultFramedSerial` throws exceptions that `FramedSerial` does not announce: [The Exception Contract](#the-exception-contract). A mock of `FramedSerial` that follows the header cannot reproduce an unplugged device. |
| Interface segregation | Weak | `Serial` joins the 5 calls the framing needs with 6 of configuration. |
| Dependency inversion | Kept | `DefaultFramedSerial` takes a `Serial` in its constructor; both users take a `FramedSerial`. |

## Coupling and Duplication

The protocol exists three times, once on the host and once in each firmware, kept in step by hand. `diff` of the copies, after renaming:

| What | Host | The two firmwares | State |
|---|---|---|---|
| `0x7E`, `0x7D`, `0x20` | private in `default_framed_serial.cpp` | private in each `SerialPort.cpp` | Equal. |
| The CRC table, 256 values | `crc_utils.cpp` | each `CrcUtils.cpp` | Equal, all three. |
| Writing a frame | `write()` and `add_escaped_byte()` | `write()` and `addByte()` | Equal, down to the two variables named the wrong way round, `crc_msb` for the low byte. |
| The reading state machine | `read()` | `update()` | **Diverged.** The firmware resynchronises, the host throws: [The Reader](#the-reader). |
| `Buffer<T>` | `buffer.hpp`, a `std::vector` that is reserved | `Buffer.h`, a `new T[capacity]` | **Diverged.** The firmware's storage is valid and its `remove()` of an empty buffer returns `T{}`; the host's has neither: issues 3 and 11. |
| The size of the buffers | 256 | 200 | Different, and nothing says which is the limit of a message: a frame the host accepts can be cut by the device. |
| The mock of `FramedSerial` | none shipped | `mock_framed_serial.hpp` in each user's tests | Two files equal but for the namespace. |

The two firmware copies of `SerialPort.cpp` are equal but for a `reset()` that only the Freezer has. Weight: medium. Nothing fails when the copies drift, and the two that have drifted are the two largest findings of this review.

Both users pin the library at the commit reviewed, in their `.repos` files. One value is out of step in a neighbour, from reading: `stepit.ros2_control.xacro` of StepIt Motors writes the hardware parameter `baud_rate`, and `DefaultSerialFactory` reads `baudrate`, so the baud rate set at launch never reaches `Serial::set_baudrate()`. It has no effect on a USB port, which ignores the rate.

## The Build

| What | Finding |
|---|---|
| The C++ standard | `CMAKE_CXX_STANDARD 23` (line 23). Nothing needs it: the 4 sources compile as C++17 with the package's own warnings, and the headers compile as C++17 and not as C++14, for the nested namespace of `data_utils.hpp`. Verified. The standard is set for the directory and exported to nobody, so a user learns the requirement from a compiler error. |
| The flags | Eight warnings and `-Werror` for every build (line 12), a user's included: a new compiler with a new warning stops the build of whoever depends on the library. |
| `rclcpp` | A public, exported dependency, for one `RCLCPP_ERROR`: every user finds and links `rclcpp`, and the library cannot be built outside a ROS2 workspace. The architecture document judges it a trade-off; we judge it a poor one, since `rcutils` logging, or the bytes carried in the exception, would do. |
| `serial` | A static library, linked into `libframed_serial.so`, which then exports its 129 `serial::` functions. It works; a program that also links `serial` itself would hold it twice. From reading the symbols. |
| The versions | `0.1.0` in `CMakeLists.txt` (line 4), `0.0.0` in `package.xml` (line 5). |
| The source list | `default_framed_serial.hpp` and `default_framed_serial.cpp` are each listed twice in `add_library()` (lines 53 and 58, 61 and 64). CMake ignores it. |
| The linters | **Never registered.** `ament_lint_auto_find_test_dependencies()` is called in [`tests/CMakeLists.txt`](../tests/CMakeLists.txt) (line 6); it is a macro that records what it found in the variables of that folder, and `ament_package()` reads them in the folder above, where they are empty. Verified: `colcon test` runs 3 tests. With the call moved to the top `CMakeLists.txt`, in a scratch copy, it runs 9, and 2 fail: `cpplint`, with 19 errors, 11 of them the `else` on its own line that `.clang-format` produces, and `uncrustify`, on 16 files, since the code is formatted with `clang-format`. `cppcheck` passes by not running: version 2.13 is skipped by ament. |
| `package.xml` | `ament_clang_format` and `ament_clang_tidy` are test dependencies that nothing uses: the `tidy` target has no `.clang-tidy` file to read. |
| CI | None at the commit reviewed; `main` now has [`industrial_ci.yml`](../.github/workflows/industrial_ci.yml), [`ci-ros-lint.yml`](../.github/workflows/ci-ros-lint.yml) and [`ci-format.yml`](../.github/workflows/ci-format.yml), added after it. Each user's CI builds the library as an upstream workspace, without its tests as far as its workflow file tells, and a workspace that builds it with `-DBUILD_TESTING=OFF` never compiles them. Until those workflows, the 19 tests ran when someone remembered. |

## Testability and Tests

The seams are good: `DefaultFramedSerial` takes its port, so every finding above was reproduced with a 60-line fake and no hardware. What is missing is use of them.

| Part | Cases | What they leave out |
|---|---|---|
| `test_crc_utils` | 1 | Nothing that matters. |
| `test_data_utils` | 8 | `to_hex()` of 16-bit values, `stoui16()`, `stoui32()`. |
| `test_default_framed_serial` | 10 | See below. |
| `Buffer` | 0 | Everything: a first `add()`, a full buffer, an empty one, a wrap, a copy. |
| `DefaultSerial` | 0 | Everything. A pseudo-terminal is enough to test it without a device: our probe sent a frame each way and timed a timeout in 0.2 s. |

What the 10 cases of the framing leave out:

- **A second call.** Every test builds a new object and calls `read()` or `write()` once. Recovery, the subject of [The Reader](#the-reader), cannot fail a test that never reads twice.
- **The limits**: a message of 0 bytes, of 252, of 253; a frame of 300.
- **A `0x7D` in the data**, an escape followed by a delimiter, a timeout in the middle of a frame.
- **Exceptions of the port**: `MockSerial` never throws.
- **The device.** No test vector is shared with a firmware: the frames of the tests are typed by hand in both places.

Three weaknesses of the tests themselves. The `read` test passes by accident: its frame carries the CRC `0x1705` of the message of the `write` test, `00 71 01 00 00 0B B8`, without that message's leading `00`, and is valid only because a leading zero does not change the CRC. The lambda that plays a frame, `buffer[0] = *it++`, is copied 5 times and reads past the end of its vector if the reader asks for one byte too many. And the names say what is called, `read`, `write_error`, not what is expected.

The library ships no fake and no mock of `FramedSerial`, so each user wrote the same one.

## Implementation Issues

Ordered by severity. **Safety** says whether the issue can make a robot driven through the library move, or keep moving, against its commands. The firmware of StepIt Motors stops every motor when no good frame has arrived for 1 s (`TIMEOUT_MS` in its `main.cpp`); several answers rest on it.

1. **A damaged reply puts the reader out of step for good.** Verified. `read()` (lines 94 and 108) consumes one or two bytes per call and never looks for the next delimiter. After a reply without its start delimiter, with a stray delimiter, or cut in two by a timeout, 0 of the next 250 requests are answered and 4258 bytes wait unread. For whom: every caller; the run in progress is lost, and the port must be closed. **Safety: no.** The caller sees an exception at every call and stops sending; the firmware's watchdog stops the motors 1 s later.
2. **Nothing drops what waits in the port, so a late or stray reply is taken for the next answer.** Verified. After one stray byte or one reply later than the timeout, each `read()` returns, with a good CRC, the reply to the request before: 250 of 250. `Serial` has no call to flush its input and `FramedSerial` none to ask for it. **Safety: not through the library**, which returns only frames the device sent whole. What a caller does with an answer that belongs to the request before is for its own review: a status one cycle old is harmless, an acknowledgement read from another reply is checked by its length at most.
3. **`Buffer` writes into a vector that has no elements.** Verified. The constructor calls `reserve()` (line 77 of [`buffer.hpp`](../include/framed_serial/buffer.hpp)), never `resize()`, and `add()` and `remove()` use `operator[]` (lines 104, 117, 135, 140) on a vector whose size is 0: undefined behaviour. As built today, GCC 13.3 with neither assertions nor annotations, it works, at `-O0` and at `-O2`, and `-fsanitize=address,undefined` reports nothing. With `-D_GLIBCXX_ASSERTIONS` the library's own first framing test aborts in `write()`, exit code 134; with AddressSanitizer and `-D_GLIBCXX_SANITIZE_VECTOR` the same test stops on `container-overflow` in `Buffer::add()`. Neither flag is set by the package or by the two users' builds. GCC 15 turns the assertions on for builds without optimisation, by its release notes, not run here: a debug build on a newer distribution then aborts at the first `write()`. **Safety: no.** The process ends when it connects, before any motor is commanded.
4. **A `read()` can last without bound.** Verified. 722 ms under a timeout of 50 ms for a frame that trickles in, and 1,000,002 calls to the port for a frame that never ends, where a frame cannot hold more than 256 bytes. `set_timeout()` casts milliseconds to `uint32_t` (line 71 of [`default_serial.cpp`](../src/default_serial.cpp)): a negative duration becomes 4,294,966 s, 49 days, and one under a millisecond becomes 0, which makes every `read()` of a byte not yet arrived a timeout. For whom: a caller in a control loop, held in one call. **Safety: no.** A host stuck in `read()` sends nothing, and the watchdog stops the motors.
5. **A frame that does not fit is cut without an error, in both directions.** Verified. `read()` of a frame with 300 bytes of data and a good CRC returns its first 254; `write()` of 253 bytes sends 256 without an end delimiter and returns normally. Both documents say so; neither says that it breaks the rule the design is built on. Latent: the longest message of either user is far below. **Safety: no today.**
6. **`read()` and `write()` throw exceptions their headers do not name.** Verified. See [The Exception Contract](#the-exception-contract). **Safety: no.** Both users catch `std::exception`.
7. **After a write that fails halfway, or a frame cut by issue 5, the device drops the next good frame too.** Verified with the transcription of the firmware's reader: 0 messages delivered after the first good frame, 1 after the second. The cause is on both sides: the host leaves `7E 77 00 40 80 00` on the wire and sends no delimiter to close it, and the firmware spends the next start delimiter as an end. `DefaultSerial::write()` returns 0 only when the port accepts nothing within the timeout, so it needs a stalled device. **Safety: marginal, the only one.** If the frame dropped is a stop, the motors run on until the watchdog, 1 s, or until the caller's next command.
8. **The CRC does not see leading zeros, and a frame of zeros is valid.** Verified. Both protocols start every message with a byte that is never `0x00`, a command from `0x71` up or a status that is not zero, which is what protects them, not the library. **Safety: no.**
9. **An escape followed by a delimiter swallows the frame that follows.** Verified. One good frame lost after a frame cut right behind a `0x7D`. **Safety: no.**
10. **`write({})` sends a frame no reader accepts.** Verified. **Safety: no.**
11. **`Buffer::remove()` of an empty buffer returns a value that was never set.** Verified by the compiler: `-O2 -Wall` reports `'out' is used uninitialized` at line 129. Not reachable from the library, whose 5 calls are all guarded by `size()`. **Safety: no.**
12. **A copy of a `Buffer` crashes.** Verified: signal 11. The copy of a vector with no elements has no storage, and `capacity_` is copied with it. Not reachable from the library: `DefaultFramedSerial` cannot be copied. The template is public. **Safety: no.**
13. **`from_int8()` and `to_int8()` are declared and not defined.** Verified: `undefined reference` at link time, 0 symbols in the built library. **Safety: no.**
14. **The CRC error is logged under the name `DataHandler`** (line 33 of [`default_framed_serial.cpp`](../src/default_framed_serial.cpp)), a class that does not exist, at the error level, before the caller has decided whether the error matters. From reading. **Safety: no.**
15. **`from_float()` and `to_float()` are right on a little-endian host only.** From reading; their comment says so, and nothing checks it. Every host the two projects run on is little-endian. **Safety: no.**

## Suspicions a Probe Disproved

- **A reply that loses its end delimiter loses the replies after it.** It does not, in a request-and-reply exchange: that `read()` ends on `timeout` having consumed the whole reply, and the next one starts clean. 250 of 250 answered.
- **A CRC error leaves the reader out of step.** It does not, as long as both delimiters arrived: 250 of 250 answered after one damaged byte.
- **One byte a call is slow.** Not in software: 1.8 µs a byte. On a device, the measurement that exists is StepIt Motors' own, on its robot over USB: 1.6 ms on average and 2.7 ms at most for a command and a status query, in a cycle of 10 ms, against an estimated 0.4 ms with one write per frame. At a true 9,600 baud the wire itself, about 1 ms a byte, costs more than the calls. So it is a cost of about 1 ms a cycle, not a fault.
- **AddressSanitizer finds the buffer.** It does not, without `-D_GLIBCXX_SANITIZE_VECTOR`: the memory is allocated, only the vector does not know it is used.
- **The diagrams are out of date.** `bin/check-diagrams.sh` passes.

## What the Documents Say and the Code Does Not

In the [README](../README.md):

- "A reader that joins the stream in the middle of a frame waits for the next delimiter and loses nothing else." True of the firmware's reader. This library's reader throws at each byte, and does not find the next frame: issue 1.
- "a `0x7E` on the wire is always a delimiter". Not after a `0x7D`, for either reader: issue 9.
- "It throws `framed_serial::SerialException` when", followed by four cases. The exceptions of the port are missing: issue 6.
- "A longer frame reaches the device cut short, and the device drops it on its CRC." The device also drops the frame after it: issue 7.

In [docs/ARCHITECTURE.md](ARCHITECTURE.md), which is otherwise accurate, and frank about most of the above:

- "`colcon test` also runs the linters of `ament_lint_common`." It runs none: [The Build](#the-build).
- "a call is a whole message or an exception", the one idea of the Introduction. The sections Writing a Frame and Reading a Frame of the same document describe two calls that are neither.
- `CRC error`: "The whole frame; the next `read()` starts clean." Only when the frame kept both its delimiters: after issue 9 the frame consumed is the next one.
- The firmware's reader "skips every byte until a delimiter, and takes a delimiter after fewer than 3 bytes as a new start". True, and incomplete: after a bad CRC it loses the next frame.
- `stoui16()` and `stoui32()` are "the parsing of a caller's settings". No caller uses them.

## Recommendations

Ordered by value against effort. Each fits one pull request. Those that change `Serial` or `FramedSerial` break the mocks of the two users, which then move to the new commit together.

1. **Give `Buffer` real storage, and test it** (issues 3, 11, 12). `data_(capacity)` in the constructor and `T out{}` in `remove()` are two lines; replacing the template by a `std::vector` in `DefaultFramedSerial` removes 147 lines. Add the cases the table of tests lists, and build the tests once with `-D_GLIBCXX_ASSERTIONS`. Trade-off: none.
2. **Make the reader resynchronise, and let the caller drop waiting input** (issues 1, 2, 9). In `Waiting`, skip bytes up to a delimiter; treat a delimiter after fewer than 3 bytes, and one that ends a frame with a bad CRC, as the start of the next frame; abort a frame on `7D 7E`. Add `flush_input()` to `Serial`, over `serial::Serial::flushInput()`, and a call on `FramedSerial` that a driver makes before each request. Test it with a fake that plays a stream over several calls, with the seven kinds of damage of [The Reader](#the-reader). Trade-off: a caller that counted on `start delimiter missing` no longer sees it; a reply later than the next flush is still read once out of turn, which only a request number in each frame would rule out, a change of both protocols.
3. **Refuse what does not fit, and bound a read** (issues 4, 5, 10). `write()` throws before it sends a byte when the frame exceeds the buffer or the message is empty; `read()` throws `frame too long` when the buffer is full, and gives up when a whole frame has taken longer than a deadline of its own. Reject a negative timeout. Publish the largest message as a constant. Trade-off: one more setting, the deadline of a frame.
4. **One family of exceptions** (issue 6). `DefaultSerial` catches the three `serial::` types and rethrows `framed_serial::SerialException`; give that class one subclass per failure, timeout, damaged frame, port lost, and carry the bytes of a damaged frame in it instead of logging them (issue 14). Trade-off: the text of `what()` changes, which 5 tests of the library compare and the logs of the users show.
5. **Run what exists, automatically** ([The Build](#the-build)). Move `ament_lint_auto_find_test_dependencies()` to the top `CMakeLists.txt`, replace `uncrustify` by `ament_cmake_clang_format` and give `cpplint` the line length of the project, and add to the GitHub workflows, which now build and test the package plain, a run with assertions and sanitizers. Trade-off: the first run fails until 19 `cpplint` errors are settled.
6. **Send a frame in one call, closed even when it fails** (issue 7 and the cost of one byte a call). `write()` hands the whole buffer to `Serial::write()` and flushes once; when the port takes only part of it, send a delimiter before throwing. In the firmware, treat the delimiter that ends a bad frame as a possible start, the same change as recommendation 2. Trade-off: the three tests that record `buffer[0]` change, and the firmware change needs a release of each board.
7. **Tidy the package** (issues 13, 15, and the tables above). C++17 through `target_compile_features()`; `-Werror` only behind an option; one version; no duplicate in `add_library()`; `rclcpp` replaced by `rcutils` or by nothing after recommendation 4; the 9 helpers nobody calls removed, or defined and tested; `const` on `FramedSerial::is_open()`; `add_escaped_byte()` in an unnamed namespace. Trade-off: removing a public function is a break for a user we do not know of.
8. **Ship what users copy** ([Coupling and Duplication](#coupling-and-duplication)). An installed header with the three constants of the frame and the largest message, a fake of `FramedSerial` for the users' tests, and a file of test frames that the firmwares' own tests can read. Trade-off: the fake becomes interface, and must be kept.
