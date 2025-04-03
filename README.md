# ESPAura Firmware Release v0.2.0

## Introduction

ESPAura is a modular and efficient firmware designed for an ESP-based Ambilight system. It leverages the Adalight protocol for seamless integration with HyperHDR and provides manual configuration via serial commands. ESPAura is perfect for users who want to create a customized Ambilight experience with easy configuration and flexibility.

## Features

- **Adalight Protocol Support**: Compatible with HyperHDR for responsive Ambilight effects.
- **EEPROM Configuration**: Stores LED count and power settings persistently.
- **Serial Command Control**: Change LED count, power settings, and reset via serial interface.
- **Boot Effect**: (Optional) Display a startup effect upon powering on.
- **LED Chase Effect**: Visual feedback when in configuration mode.
- **WiFi-Ready (Future Feature)**: Planned integration for web-based control and MQTT support.
- **Manual Mode Toggle**: Easily switch between Adalight and manual mode using a configuration button.

## Serial Commands

| Command        | Description             |
| -------------- | ----------------------- |
| `!SET LEDS, X` | Set LED count (1-800)   |
| `!SET MA, X`   | Set max milliamps       |
| `!RESET`       | Restart ESP             |
| `!ADA`         | Switch to Adalight mode |

*Example*: To set the LED count to 200, send `!SET LEDS, 200` in your serial monitor.

## How to Flash the Firmware

1. Connect your ESP device via USB.
2. Use ESPHome flasher: [ESPHome Flasher](https://github.com/esphome/esphome-flasher).
3. Open a serial monitor at `921600` baud to verify operation.

## Future Improvements

- **WiFi Access Point for Configuration** (Planned for next release)
- **Infrared Remote Support** (Coming soon)
- **MQTT Control Integration** (To be added in a future version)

## Changelog

### v1.0.0 - Initial Release

- Core functionalities implemented.
- Adalight, EEPROM, and Serial Control.
- Chase effect for visual feedback.

---

Developed by **Esteladac** | ESPAura Project
