# Bluetooth Head Tracker Receiver

A Bluetooth Low Energy (BLE) head tracker receiver based on ESP32-C3 that converts head tracking data from OpenTX/EdgeTX transmitters into PPM (Pulse Position Modulation) signals for FPV drone control.

## Features

- **Bluetooth Low Energy (BLE) Connectivity**: Receives head tracking data from OpenTX/EdgeTX compatible transmitters
- **PPM Signal Output**: Converts received data into standard PPM signals for flight controller input
- **Multi-channel Support**: Supports up to 8 channels of head tracking data
- **FreeRTOS Task Management**: Efficient multi-tasking with separate tasks for Bluetooth, LED control, and data synchronization
- **LED Status Indication**: Visual feedback for connection status and operation state
- **ESP32-C3 Platform**: Built on the cost-effective ESP32-C3 development board

## Hardware Requirements

- ESP32-C3-DevKitC-02 development board
- PPM signal output circuit (connected to specified GPIO pins)
- LED indicators for status display
- Power supply (5V via USB or external)

## Software Requirements

- PlatformIO IDE or VS Code with PlatformIO extension
- ESP32 platform support
- Arduino framework

## Pin Configuration

- **PPM Output**: GPIO 6 (configurable)
- **LED Control**: Configurable via LED task
- **Serial Debug**: USB CDC on boot enabled

## Installation

1. Clone this repository or download the source code
2. Open the project in PlatformIO
3. Install required dependencies (if any)
4. Build the project
5. Upload to your ESP32-C3 development board
6. Connect the PPM output to your flight controller's trainer port

## Usage

1. Power on the ESP32-C3 board
2. The device will start advertising via BLE
3. Pair with your OpenTX/EdgeTX transmitter
4. Configure head tracking on your transmitter
5. The device will output PPM signals corresponding to head movements

## Project Structure

```
BlueToothHeadTrackerReceiver/
├── src/
│   ├── main.cpp          # Main application with FreeRTOS task creation
│   ├── bluetooth.cpp     # BLE communication and data reception
│   ├── ppmout.cpp        # PPM signal generation and output
│   ├── led.cpp           # LED status indication
│   └── opentxbt.cpp      # OpenTX/EdgeTX protocol handling
├── include/
│   ├── bluetooth.h       # Bluetooth task and data structures
│   ├── ppmout.h          # PPM output function declarations
│   ├── led.h             # LED control definitions
│   └── opentxbt.h        # OpenTX protocol definitions
├── platformio.ini        # PlatformIO project configuration
└── README files          # Documentation
```

## Configuration

### PPM Settings
- **Channels**: 8 channels supported
- **Frame Length**: Configurable via `setPPMFrameLength()`
- **Sync Length**: Configurable via `setPPMSyncLength()`
- **Output Pin**: Configurable in `ppm_init()`

### Bluetooth Settings
- **Device Name**: Configurable in bluetooth.cpp
- **Service UUID**: Defined for OpenTX/EdgeTX compatibility
- **Characteristic UUID**: For head tracking data reception

## FreeRTOS Tasks

1. **Bluetooth Task**: Handles BLE connection and data reception
2. **LED Task**: Manages status LED indicators
3. **Main Loop**: Minimal processing, primarily for task oversight

## Supported Protocols

- OpenTX/EdgeTX trainer protocol over BLE
- Standard PPM signal format
- 8-channel head tracking data

## Troubleshooting

### Connection Issues
- Ensure BLE is enabled on your transmitter
- Check that the device is advertising
- Verify UUID compatibility with your transmitter

### PPM Signal Issues
- Check wiring connections to flight controller
- Verify PPM frame timing settings
- Use oscilloscope to debug signal quality

### Build Issues
- Ensure ESP32 platform is properly installed
- Check USB CDC configuration for serial output
- Verify GPIO pin assignments match your hardware

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## License

This project is open source. Please check the repository for specific license information.

## Acknowledgments

- OpenTX/EdgeTX community for protocol specifications
- ESP32 Arduino community for platform support
- FPV community for testing and feedback