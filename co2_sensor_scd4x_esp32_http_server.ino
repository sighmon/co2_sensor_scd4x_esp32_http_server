/*
 Arduino ESP32 WiFi Web Server for the Adafruit SCD4X CO2 sensor.
 Responds to http requests with prometheus.io syntax responses.

 # HELP ambient_temperature Ambient temperature
 # TYPE ambient_temperature gauge
 ambient_temperature 31.52
 # HELP ambient_humidity Ambient humidity
 # TYPE ambient_humidity gauge
 ambient_humidity 52.83
 # HELP co2 CO2
 # TYPE co2 gauge
 co2 670
 # HELP battery_voltage Battery voltage
 # TYPE battery_voltage gauge
 battery_voltage 3.15

 Based on:
 * ESP32 example code SimpleWiFiServer by Jan Hendrik Berlin
 * Sensirion I2C SCD4X example code exampleUsage Copyright (c) 2021, Sensirion AG
 * Sensirion Example8_SCD4x_BLE_Gadget_with_RHT

 Written by Simon Loffler on invasion/survival day 26/1/2022
*/

#include "secrets.h"
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL);

// Task scheduler
#include <TaskScheduler.h>
void readSensorCallback();
Task readSensorTask(5000, -1, &readSensorCallback);  // Read sensor every 5 seconds
Scheduler runner;

// BLE
#include "DataProvider.h"
#include "NimBLELibraryWrapper.h"
NimBLELibraryWrapper lib;
DataProvider provider(lib, DataType::T_RH_CO2);

// SCD4X sensor init
#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>

SensirionI2CScd4x scd4x;
uint16_t error;
char errorMessage[256];
uint16_t co2;
float temperature;
float humidity;
float voltage;

// Task callback
void readSensorCallback() {
    // Read the voltage
    int sensorValue = analogRead(A2);
    voltage = sensorValue * (5.0 / 4096.0);

    // Read the SCD4X CO2 sensor
    error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error) {
        printToSerial("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        printToSerial(errorMessage);
    } else if (co2 == 0) {
        printToSerial("Invalid sample detected, skipping.");
    } else {
        printToSerial((String)"Co2: " + co2);
        printToSerial((String)"Temperature: " + temperature);
        printToSerial((String)"Humidity: " + humidity);
        printToSerial((String)"Voltage: " + voltage);
        printToSerial("");
    }

    // Report sensor readings via BLE
    provider.writeValueToCurrentSample(co2, Unit::CO2);
    provider.writeValueToCurrentSample(temperature, Unit::T);
    provider.writeValueToCurrentSample(humidity, Unit::RH);
    provider.commitSample();
    provider.handleDownload();

    // Pulse blue LED
    pixels.setPixelColor(0, pixels.Color(0, 0, 10));
    pixels.show();
    pixels.clear();
    pixels.show();
}

void printUint16Hex(uint16_t value) {
    Serial.print(value < 4096 ? "0" : "");
    Serial.print(value < 256 ? "0" : "");
    Serial.print(value < 16 ? "0" : "");
    Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
    Serial.print("Serial: 0x");
    printUint16Hex(serial0);
    printUint16Hex(serial1);
    printUint16Hex(serial2);
    Serial.println();
}

void printToSerial(String message) {
  // Check for Serial to avoid the ESP32-C3 hanging attempting to Serial.print
  if (Serial) {
    Serial.println(message);
  }
}

// WiFi init
#include <WiFi.h>
char* ssid     = SECRET_SSID;
char* password = SECRET_PASSWORD;
WiFiServer server(80);

void setup() {

    Serial.begin(115200);
    delay(100);

    // SCD4X setup
    Wire.begin();

    uint16_t error;
    char errorMessage[256];

    scd4x.begin(Wire);

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
        printToSerial("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        printToSerial(errorMessage);
    }

    uint16_t serial0;
    uint16_t serial1;
    uint16_t serial2;
    error = scd4x.getSerialNumber(serial0, serial1, serial2);
    if (error) {
        printToSerial("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        printToSerial(errorMessage);
    } else {
        if (Serial) {
          printSerialNumber(serial0, serial1, serial2);
        }
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        printToSerial("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        printToSerial(errorMessage);
    }

    printToSerial("Waiting for first measurement... (5 sec)");

    // Setup read sensor task
    readSensorTask.enable();
    runner.addTask(readSensorTask);
    runner.enableAll();

    // Setup BLE
    provider.begin();
    printToSerial("Sensirion BLE Lib initialized with deviceId: " + provider.getDeviceIdString());

    // WiFi setup
    pixels.begin();
    pixels.setPixelColor(0, pixels.Color(10, 0, 0));
    pixels.show();
    delay(500);
    pixels.clear();
    pixels.show();

    delay(10);

    // We start by connecting to a WiFi network
    printToSerial((String)"Connecting to " + ssid);

    WiFi.begin(ssid, password);

    // Wait for a WiFi connection for up to 5 seconds
    for (int i = 0; i < 5; i++) {
        if (WiFi.status() != WL_CONNECTED) {
            pixels.setPixelColor(0, pixels.Color(0, 0, 10));
            pixels.show();
            delay(500);
            pixels.clear();
            pixels.show();
            printToSerial(".");
            delay(500);
        } else {
            printToSerial("WiFi connected.");
            printToSerial("IP address: ");
            printToSerial((String)WiFi.localIP());

            pixels.setPixelColor(0, pixels.Color(0, 10, 0));
            pixels.show();
            delay(500);
            pixels.clear();
            pixels.show();
            break;
        }
    }
    server.begin();
}

void loop() {
  runner.execute();

  // WiFi server
  
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    printToSerial("New Client.");           // print a message out the serial port
    printToSerial("");
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        // printToSerial("" + c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html; charset=UTF-8");
            client.println();

            // Pulse the LED to show a connection has been made
            pixels.setPixelColor(0, pixels.Color(0, 10, 0));
            pixels.show();

            // Send Prometheus data
            client.print("# HELP ambient_temperature Ambient temperature\n");
            client.print("# TYPE ambient_temperature gauge\n");
            client.print((String)"ambient_temperature " + temperature + "\n");
            client.print("# HELP ambient_humidity Ambient humidity\n");
            client.print("# TYPE ambient_humidity gauge\n");
            client.print((String)"ambient_humidity " + humidity + "\n");
            client.print("# HELP co2 CO2\n");
            client.print("# TYPE co2 gauge\n");
            client.print((String)"co2 " + co2 + "\n");
            client.print("# HELP battery_voltage Battery voltage\n");
            client.print("# TYPE battery_voltage gauge\n");
            client.print((String)"battery_voltage " + voltage + "\n");

            pixels.clear();
            pixels.show();

            // The HTTP response ends with another blank line:
            client.print("\n");
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // close the connection:
    client.stop();
    printToSerial("Client Disconnected.");
  }
}
