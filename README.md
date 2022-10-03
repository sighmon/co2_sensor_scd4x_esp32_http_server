# Adafruit SCD4X CO2 sensor for ESP32

An Arduino sketch for the [Adafruit SCD4X CO2 sensor](https://github.com/adafruit/Adafruit_CircuitPython_SCD4X) and [Adafruit SCD-30 CO2 sensor](https://github.com/adafruit/Adafruit_SCD30) running on an ESP32 with an http server returning [Prometheus](https://prometheus.io) compatible responses.

![The Adafruit SCD-41 CO2 sensor graphed in Grafana](scd-41-co2-temperature-humidity.png)

Related software:

* [Prometheus/Grafana Docker containers for Raspberry Pi](https://github.com/sighmon/prometheus-grafana-raspberry-pi)
* [Apple HomeKit accessory for the SCD4x/SCD-30 CO2 sensor](https://github.com/sighmon/homekit-scd4x)
* [iOS/macOS BLE CO2 sensor](https://github.com/sighmon/ios-ble-co2-sensor)

## Setup

**WiFi**:

* Copy the secrets file: `cp secrets.tmpl.h secrets.h`
* Fill in your SSID and password in `secrets.h`
* **Note**: my [ESP32 Thing](https://www.sparkfun.com/products/13907) would only connect to a 2.4 GHz network

**Arduino**:

* Download the latest version of Arduino from https://www.arduino.cc
* From `Tools > Manage Libraries...` add the library: `Sensirion I2C SCD4x`
* If you have an `SCD-30` CO2 sensor, add the library: `Adafruit SCD30`
* Connect your ESP32 via USB
* From `Tools > Board` choose your ESP32 board, port, and speed: `115200`
* Now try `Upload` to send the code to your ESP32

**Note**: If you have an `SCD-30` CO2 sensor, make sure you're on the branch [add/2-scd-30](https://github.com/sighmon/co2_sensor_scd4x_esp32_http_server/tree/add/2-scd-30) before uploading the sketch: `git checkout add/2-scd-30`

**iOS/macOS Bluetooth app**:

I built an iOS/macOS app so location data can be saved with CO2 reading data.

Project code: [ios-ble-co2-sensor](https://github.com/sighmon/ios-ble-co2-sensor)

<img src="https://github.com/sighmon/ios-ble-co2-sensor/raw/main/co2-sensor-ios-green.png" width="20%" style="padding: 10px;"><img src="https://github.com/sighmon/ios-ble-co2-sensor/raw/main/co2-sensor-ios-yellow.png" width="20%" style="padding: 10px;"><img src="https://github.com/sighmon/ios-ble-co2-sensor/raw/main/co2-sensor-ios-orange.png" width="20%" style="padding: 10px;"><img src="https://github.com/sighmon/ios-ble-co2-sensor/raw/main/co2-sensor-ios-archive-detail.png" width="20%" style="padding: 10px;">

This code also works with the standard Sensirion MyAmbience app:

* Install the Sensirion MyAmbience app: [iOS](https://apps.apple.com/ch/app/sensirion-myambience-co2/id1529131572)/[Android](https://play.google.com/store/apps/details?id=com.sensirion.myam)
* Turn on Bluetooth on your device
* The readings should update every 5 seconds

## Serial monitor

If you open the Arduino serial monitor you'll see:

* The WiFi output as it attempts to connect to your WiFi network
* The http response including the SCD4X/SCD-30 CO2 readings

## http response

Once your ESP32 has connected to your SSID, it will respond over port 80 with a [Prometheus](https://prometheus.io) response. e.g. `curl 192.168.1.100:80/metrics`

```bash
# HELP ambient_temperature Ambient temperature
# TYPE ambient_temperature gauge
ambient_temperature 29.70
# HELP ambient_humidity Ambient humidity
# TYPE ambient_humidity gauge
ambient_humidity 55.85
# HELP co2 CO2
# TYPE co2 gauge
co2 627
```

My ESP32 Thing with the SCD-41 CO2 sensor running off of two recycled 18650 laptop batteries.

![My ESP32 Thing with the SCD-41 CO2 sensor running off of two recycled 18650 laptop batteries](scd4x-esp32-thing.jpg)

## Thanks

Shoutout to [Core Electronics](https://core-electronics.com.au) for being so fast with their delivery of the [Adafruit SCD-41](https://core-electronics.com.au/adafruit-scd-41-ndir-co2-temperature-and-humidity-sensor-stemma-qt-qwiic.html).

## Licenses

* My code: MIT License
* ESP32 example code `SimpleWiFiServer` by Jan Hendrik Berlin
* Sensirion I2C SCD4X example code `exampleUsage` license:

Copyright (c) 2021, Sensirion AG
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
* Neither the name of Sensirion AG nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
