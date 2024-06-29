# HomeKit SI7021 Temperature and Humidity Sensor

This project implements a HomeKit-compatible temperature and humidity sensor using an ESP32 microcontroller and an SI7021 sensor. The sensor readings are exposed via HomeKit, allowing you to monitor temperature and humidity from your Apple devices.

## Features

- Temperature and humidity measurements using SI7021 sensor
- HomeKit integration for easy access from Apple devices
- Periodic sensor readings (every 10 seconds)
- Wi-Fi connectivity
- Factory reset functionality

## Hardware Requirements

- ESP32-WROOM development board. This is the one I used https://www.aliexpress.com/item/1005006888612002.html
- SI7021 temperature and humidity sensor. This is the one I used https://www.aliexpress.com/item/1005006583390274.html
- Push button for reset functionality (connected to GPIO 0)

## Software Dependencies

This project uses the following libraries and frameworks:

- ESP-IDF (Espressif IoT Development Framework)
- HomeKit SDK (HomeKit Software Development Kit)
- esp-idf-lib (for SI7021 sensor driver)

## Setup and usage

1. Clone this repository:
   ```
   git clone https://github.com/yourusername/homekit-si7021-sensor.git
   cd homekit-si7021-sensor
   ```

2. Update git submodules:
   ```
   git submodule update --init --recursive
   ```

3. Delete components/esp-idf-lib/components/button as it conflicts with the HomeKit SDK
   '''
   rm -rf components/esp-idf-lib/components/button
   del components\esp-idf-lib\components\button
   '''

4. Build and flash the project:
   ```
   idf.py build
   idf.py -p (YOUR_PORT) flash
   ```

5. Monitor the output:
   ```
   idf.py monitor
   ```
6. Provision the device with ESP BLE Provisioning https://apps.apple.com/us/app/esp-ble-provisioning/id1473590141

7. Use the Home app on your Apple device to scan the HomeKit setup code (111-22-333) and add the accessory.

## Customization

- To change the HomeKit setup code, modify the `SETUP_CODE` define in the source code.
- To change the device name, modify the `cfg.name` field in the `hap_init_task` function.

## Troubleshooting

- If you're having trouble connecting to WiFi, check your WiFi credentials in the project configuration.
- If the sensor readings aren't updating, ensure that the SI7021 sensor is correctly connected to the I2C pins (SDA: GPIO 21, SCL: GPIO 22).

## Contributing

Contributions to this project are welcome. Please feel free to submit a Pull Request.

