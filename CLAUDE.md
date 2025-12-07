<!-- OPENSPEC:START -->
# OpenSpec Instructions

These instructions are for AI assistants working in this project.

Always open `@/openspec/AGENTS.md` when the request:
- Mentions planning or proposals (words like proposal, spec, change, plan)
- Introduces new capabilities, breaking changes, architecture shifts, or big performance/security work
- Sounds ambiguous and you need the authoritative spec before coding

Use `@/openspec/AGENTS.md` to learn:
- How to create and apply change proposals
- Spec format and conventions
- Project structure and guidelines

Keep this managed block so 'openspec update' can refresh the instructions.

<!-- OPENSPEC:END -->

# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is the Stanford CS144 "Sponge" project - a userspace TCP implementation. The project is organized into labs (lab0-lab4) that build up a complete TCP stack.

## Build System

This project uses CMake. All commands should be run from the `build` directory.

### Initial Setup
```bash
mkdir -p build && cd build
cmake ..
```

### Building
```bash
make                    # Build all targets
make -j$(nproc)        # Parallel build
```

### Testing
```bash
make check_lab0        # Test lab0 (byte stream)
make check_lab1        # Test lab1 (stream reassembler)
make check_lab2        # Test lab2 (TCP receiver)
make check_lab3        # Test lab3 (TCP sender)
make check_lab4        # Test lab4 (TCP connection)
make check_webget      # Test webget application
```

### Other Useful Commands
```bash
make format            # Format code with clang-format
make tidy              # Run clang-tidy
make cppcheck          # Run cppcheck
make doc               # Generate Doxygen documentation
```

## Architecture

### Core Components

The TCP implementation is factored into several key components in `libsponge/`:

1. **ByteStream** (`byte_stream.hh/cc`): Basic in-order byte stream abstraction. Used as the foundation for both sending and receiving data.

2. **StreamReassembler** (`stream_reassembler.hh/cc`): Reassembles out-of-order TCP segments into an in-order byte stream. Handles overlapping segments and maintains a buffer of unassembled data.

3. **TCPReceiver** (`tcp_receiver.hh/cc`): Handles inbound TCP segments. Uses StreamReassembler to reconstruct the byte stream and computes acknowledgment numbers and window sizes to advertise back to the sender.

4. **TCPSender** (`tcp_sender.hh/cc`): Handles outbound data. Breaks the byte stream into segments, manages the retransmission timer, and handles congestion control. Tracks outstanding (in-flight) segments.

5. **TCPConnection** (`tcp_connection.hh/cc`): Combines a TCPReceiver and TCPSender to implement a complete TCP endpoint. Manages the connection state machine (SYN, FIN, RST handling).

### Helper Components

- **WrappingInt32** (`wrapping_integers.hh/cc`): Handles 32-bit sequence number arithmetic with wrapping
- **TCP Helpers** (`libsponge/tcp_helpers/`): TCP segment, header, and configuration structures
- **Utilities** (`libsponge/util/`): File descriptors, sockets, TUN/TAP devices, event loops

### Key Design Patterns

1. **Pull-based I/O**: The TCP implementation uses a pull-based model. Applications write data to the sender's byte stream, and the network layer pulls segments via `segments_out()`. Similarly, received segments are pushed in via `segment_received()`, and applications read from the receiver's byte stream.

2. **Explicit State Management**: Each component manages its own state explicitly. The TCPConnection coordinates between sender and receiver to implement the TCP state machine.

3. **Timer Management**: The TCPSender manages a single retransmission timer for all outstanding segments, following TCP's exponential backoff rules.

## Testing

The `tests/` directory contains comprehensive tests for each lab:

- **Lab 0**: Byte stream functionality
- **Lab 1**: Stream reassembler (handling out-of-order segments)
- **Lab 2**: TCP receiver (acknowledgments, window management)
- **Lab 3**: TCP sender (retransmission, congestion control)
- **Lab 4**: TCP connection (state machine, connection establishment/teardown)

Each test executable can be run individually or via `make check_labN`.

## Applications

The `apps/` directory contains several applications built on the TCP stack:

- **webget**: Simple HTTP client
- **tcp_ipv4**: TCP over IPv4
- **tcp_udp**: TCP over UDP (for testing)
- **tcp_native**: Native TCP sockets
- **tun**: TUN/TAP interface
- **tcp_benchmark**: Performance testing

## Important Implementation Notes

1. **Sequence Numbers**: TCP uses 32-bit wrapping sequence numbers. Use WrappingInt32 for all sequence number arithmetic.

2. **Window Management**: The receiver's window size is based on available capacity in the ByteStream. The sender must respect the receiver's advertised window.

3. **SYN/FIN Handling**: SYN and FIN flags occupy one byte in the sequence space each. They must be tracked as part of bytes_in_flight.

4. **Retransmission Timeout**: The initial RTO is configurable (default 1000ms). It doubles after each consecutive retransmission (exponential backoff).

5. **State Machine**: TCPConnection implements the full TCP state machine including simultaneous open/close scenarios.

6. **Linger**: After both streams finish, the connection lingers for 10 * rt_timeout to handle potential retransmissions from the peer.
