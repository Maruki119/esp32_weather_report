# ESP32 WITH MQTT
# Weather Report

## Overview
This project is an Arduino-based weather reporting system that collects and displays environmental data such as temperature, humidity, and atmospheric pressure.

## Features
- Reads temperature, humidity, and pressure data from sensors.
- Displays the collected data on an LCD screen (or serial monitor).
- Can be extended to log data or send it wirelessly.

## Requirements
- Arduino board (e.g., Arduino Uno)
- DHT11/DHT22 for temperature and humidity sensing
- BMP180/BMP280 for pressure sensing (if applicable)
- LCD display (optional)
- Arduino IDE

## Installation
1. Clone this repository or download the `weather_report.ino` file.
2. Open the file in Arduino IDE.
3. Install the necessary libraries (`DHT`, `Adafruit_BMP085`, etc.).
4. Connect the sensors according to the wiring diagram.
5. Upload the code to your Arduino board.

## Usage
- Power on the Arduino.
- View the weather data on the serial monitor or LCD display.
- Modify the code to add data logging or wireless transmission.

## License
This project is open-source and available under the MIT License.

