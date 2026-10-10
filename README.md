# Wireless Step Counter

An embedded wireless step-counting system developed as a group project in Spring 2026.

The project integrates a Bosch BMI160 inertial measurement unit (IMU) with a Nordic nRF52 Development Kit using Zephyr RTOS. Step data is collected through I²C communication and transmitted wirelessly via Bluetooth Low Energy (BLE) to a web-based interface.

## Project Overview

The system combines embedded programming, sensor integration, wireless communication, and web development into a functional wireless step counter.

The system consists of three main components:

- **BMI160 IMU:** Detects and counts steps using its integrated accelerometer and hardware step counter.
- **Nordic nRF52 DK:** Handles sensor configuration, I²C communication, and Bluetooth data transmission.
- **Web Interface:** Displays step counts in real time and allows users to connect, disconnect, and reset the counter.

## Technologies Used

| Technology | Application |
|---|---|
| Embedded C | Firmware development |
| Zephyr RTOS | Embedded operating system |
| Nordic nRF52 DK | Microcontroller platform |
| Bosch BMI160 | Motion sensing and step detection |
| I²C | Sensor communication |
| Bluetooth Low Energy | Wireless communication |
| Nordic UART Service (NUS) | BLE data transmission |
| HTML, CSS & JavaScript | Web interface |
| CMake | Build configuration |

## System Architecture

```text
       Physical Movement
               |
               v
         +-----------+
         | BMI160 IMU|
         +-----------+
               |
               | I2C
               v
         +-----------+
         | nRF52 DK  |
         |Zephyr RTOS|
         +-----------+
               |
               | Bluetooth Low Energy
               v
         +-----------+
         | Web App   |
         |Step Count |
         +-----------+
```

The BMI160 uses its internal step-counting functionality to detect movement. The nRF52 DK periodically reads the step-count registers and transmits updated values via Bluetooth whenever the count changes.

## Key Features

- Real-time step counting
- I²C sensor communication and register configuration
- Bluetooth Low Energy connectivity
- Nordic UART Service integration
- Web-based step counter visualization
- Remote step-counter reset
- BLE connection status and communication logging
- Sensor initialization and identification

## Project Structure

```text
wireless-step-counter/
├── src/
│   └── main.c
├── boards/
│   └── nrf52dk_nrf52832.overlay
├── web/
│   └── step_counter.html
├── CMakeLists.txt
├── prj.conf
├── .gitignore
└── README.md
```

## Hardware Requirements

- Nordic nRF52 DK (nRF52832)
- Bosch BMI160 IMU module
- Jumper wires
- USB cable
- Computer with Bluetooth Low Energy support

## Software Requirements

- nRF Connect SDK / Zephyr RTOS
- Visual Studio Code with nRF Connect extension
- CMake and Zephyr build tools
- Web Bluetooth-compatible browser (Chrome or Edge)

## Testing and Results

The step counter was evaluated using three different BMI160 sensitivity configurations: sensitive, normal, and robust.

Testing involved 100 manually counted steps for each configuration across three walking styles: light, normal, and aggressive.

The results showed that sensitive mode provided consistent detection across different walking styles, while robust mode was less effective during light walking. Normal and aggressive walking generally produced accurate results across all three configurations.

## Project Information

**Type:** Group Project  
**Completed:** Spring 2026  
**Focus:** Embedded Systems, Sensor Integration, Wireless Communication

## Contributors

Developed as a group project.

