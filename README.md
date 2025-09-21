## Controller Firmware for ICHack-Badge board (Raspberry Pi Pico)

### Overview

This repository contains source code of the firmware for controllers, designed to run on [ICHack Badge board](https://github.com/ICRS/IC-Hack-Badge) (similar to Raspberry Pi Pico) with ESP01 for WiFi communication on a wireless controller:

*   **`remote_controller/`**: Contains the source code and build files for the wireless remote controller.
*   **`usb_controller/`**: Contains the source code and build files for the USB-tethered controller.
*   **`esp_bridge/`**: Contains the source code and build files for the ESP01 bridge that allows to use the microprocessor as a bridge to communicate with ESP01 using AT commands (over USB serial io of the board). It was only used for testing purposes.

Each directory is a self-contained CMake project that requires the Raspberry Pi Pico C/C++ SDK to compile.

_(The `CMakeLists.txt` and `pico_sdk_import.cmake` files were generated using the official Raspberry Pi Pico extension for VS Code on Windows 11.)_

The actual boards used are designed by [ICRS](https://github.com/ICRS), repository to their hardware specifications can be found [here](https://github.com/ICRS/IC-Hack-Badge).

### Wi-Fi module

To connect the board to a local network I used an ESP8266 Serial Wi-Fi Wireless Transceiver Module (ESP-01).
![Wiring diagram](/images/wiring_diagram.jpg)
The module connects to the board over its UART1 port, which enables serial communication. The module can read AT commands sent from the board and produces appropriate responses that are then processed by the main microcontroller board.

### Prerequisites

*   **Raspberry Pi Pico C/C++ SDK**
*   **CMake**
*   **ARM GCC Compiler Toolchain**

<!-- ### Building the Firmware
TODO: Must be updated to specify exact platforms

1.  Navigate to the directory of the controller you wish to build (`remote_controller` or `usb_controller`).
2.  Create and navigate into a `build` directory:
    ```bash
    mkdir build
    cd build
    ```
3.  Run CMake to generate the build files:
    ```bash
    cmake ..
    ```
4.  Run Make to compile the firmware. The output will be a `.uf2` file.
    ```bash
    make
    ``` -->

### Debugging

The firmware can be configured for easier debugging by modifying its `CMakeLists.txt` file.

#### Enabling USB Serial Output (stdio)

By default, the Pico's standard I/O (**stdio**) streams are routed to **UART0**. To read serial output (e.g., from `printf`) directly over the controller's USB connection, you must redirect **stdio** to USB.

In `CMakeLists.txt`, ensure these two lines are configured as follows:

```cmake
pico_enable_stdio_usb(project_name 1)
pico_enable_stdio_uart(project_name 0)
```
where project_name is the name of your _specific_ project.

#### Enabling Verbose Debug Logging

The source code contains extensive diagnostic logging that is compiled only when the `DEBUG` preprocessor macro is defined. This allows you to enable verbose logging for development without impacting the performance or size of the final release build.

Uncomment the following line in `CMakeLists.txt` to compile for a DEBUG build:

```cmake
add_compile_definitions(usb_controller PUBLIC DEBUG=1)
```
