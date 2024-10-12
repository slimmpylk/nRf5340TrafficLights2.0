# Traffic Light Control System - NRF5340 Project

This project is an embedded traffic light control system for the NRF5340 platform. It provides a mechanism to control traffic light sequences through UART commands. The system is built using the Zephyr RTOS and implements several key features including parsing time strings, command sequencing, and using condition variables and FIFO buffers to control LED tasks in an organized manner.

## Features

- **LED Control via UART Commands**: Supports control of traffic lights (Red, Green, Yellow) using UART commands.
- **Sequence Parsing and Execution**: Parses command sequences (e.g., `G,1000,R,1000,Y,1000`) to control the duration each light is on.
- **Timer-Based Light Control**: Uses a timer to execute traffic light operations in sequence.
- **Debugging Options**: Enable or disable debugging using UART commands (`D,1` to enable, `D,0` to disable).
- **Google Test Integration**: Includes unit tests for parsing functions to ensure correctness.

## System Components

### Hardware
- **NRF5340 Development Board**: This is the target platform for the project.
- **LEDs**: Red, Green, and Yellow LEDs are used to simulate traffic lights.
- **UART Interface**: Used to receive commands from the user to control the traffic lights.

### Software
- **Zephyr RTOS**: The real-time operating system used to develop the embedded software.
- **C/C++ Codebase**: The main control logic is implemented in C, and the unit tests are in C++ using Google Test.
- **Google Test Framework**: Integrated for unit testing parsing functions like `time_parse` and `sequence_parse`.

## Project Structure
```
├── src
│   ├── main.c            # Main application code for traffic light control
│   ├── TimeParser.h      # Header file for time parsing functions
│   └── TrafficParser.h   # Header file for sequence parsing functions
├── tests
│   ├── TimeParserTest.cpp  # Google Test unit tests for time parsing
│   ├── TrafficParserTest.cpp # Google Test unit tests for sequence parsing
│   └── CMakeLists.txt     # CMake configuration for building tests
└── README.md            # Project documentation
```

## How to Build and Run

### Prerequisites
- **Nordic Semiconductor Toolchain**: Make sure you have installed the [nRF Connect SDK](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/gs_installing.html) which includes the required toolchain.
- **Google Test**: The tests require Google Test to be available in your build environment.


### Running the Tests
To run the unit tests:
1. Navigate to the `tests/build` directory:
   ```sh
   cd tests
   mkdir build && cd build
   cmake .. -G Ninja
   ninja
   ```
2. Execute the test binary:
   ```sh
   ./TimeParserTest
   ./TrafficParserTest
   ```

## Usage

- **UART Commands**:
  - `D,1` : Enable debugging messages.
  - `D,0` : Disable debugging messages.
  - **Light Sequences**: Send sequences such as `G,1000,R,1000,Y,1000` to control the lights and example `T,2` will make the sequence loop twice .
  - **Time String Parsing**: Send a time string in `HHMMSS` format, such as `000120` to set the timer for the lights, just for demo how googletest works doesnt actually put timer.

![googleTesttulokset - Copy](https://github.com/user-attachments/assets/e98319bd-e174-4de6-aadb-5a8acc0f968e)

    

### Example Output
```
UART initialized successfully
GPIOs initialized successfully
Started serial read example
UART Receive Task Started
Dispatcher Task Started
Red Light Task Started
Green Light Task Started
Yellow Light Task Started
Debugging enabled
Received command: G,1000,R,1000,Y,1000
Parsed command - Color: G, Duration: 1000 ms
Parsed command - Color: R, Duration: 1000 ms
Parsed command - Color: Y, Duration: 1000 ms
...
```
