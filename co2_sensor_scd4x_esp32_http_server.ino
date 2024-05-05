/*
 Arduino ESP32 WiFi Web Server for the Adafruit SCD4X and SCD30 CO2 sensors.
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
 * Sensirion I2C SCD4X/SCD30 example code exampleUsage Copyright (c) 2021, Sensirion AG
 * Sensirion Example8_SCD4x_BLE_Gadget_with_RHT
 * Sensirion Example2_SCD30_BLE_Gadget

 Written by Simon Loffler on invasion/survival day 26/1/2022
*/

#include "secrets.h"

// Uncomment which sensor you're using
#define USESCD30
// #define USESCD4X

// If on an ESP32-C3 set this
// int LED_BUILTIN = 13;

// Task scheduler
#include <TaskScheduler.h>
void readSensorCallback();
Task readSensorTask(5000, -1, &readSensorCallback);  // Read sensor every 5 seconds
Scheduler runner;

// BLE
#include "DataProvider.h"
#include "NimBLELibraryWrapper.h"
#include "Sensirion_Gadget_BLE.h"
NimBLELibraryWrapper lib;

#ifdef USESCD30
  // SCD30
  DataProvider provider(lib, DataType::T_RH_CO2_ALT);
#endif
#ifdef USESCD4X
  // SCD4X
  DataProvider provider(lib, DataType::T_RH_CO2);
#endif

#ifdef USESCD30
  // SCD30 sensor init
  #include <SensirionI2cScd30.h>
  SensirionI2cScd30 sensor;
#endif
#ifdef USESCD4X
  // SCD4X sensor init
  #include <SensirionI2CScd4x.h>
  SensirionI2CScd4x sensor;
#endif

// BLE
#include "DataProvider.h"
#include "NimBLELibraryWrapper.h"
NimBLELibraryWrapper lib;
DataProvider provider(lib, DataType::T_RH_CO2);

// SCD4X sensor init
#include <Arduino.h>
#include <Wire.h>

uint16_t error;
char errorMessage[256];

#ifdef USESCD30
  // SCD30
  float co2;
#endif
#ifdef USESCD4X
  // SCD4X
  uint16_t co2;
#endif
float temperature;
float humidity;
float voltage;

// Task callback
void readSensorCallback() {
    // Read the voltage (ESP32-C3 plugged into laptop 4.2V reads 3342)
    // int sensorValue = analogRead(A2);
    // printToSerial((String)"Analog read: " + sensorValue);
    // voltage = sensorValue * (4.2 / 3342.0);

#ifdef USESCD30
    // Read the SCD30 CO2 sensor
    uint16_t data_ready = 0;
    sensor.getDataReady(data_ready);
    if (bool(data_ready)) {
        error = sensor.readMeasurementData(co2, temperature, humidity);
        if (error != NO_ERROR) {
            Serial.print("Error trying to execute readMeasurementData(): ");
            errorToString(error, errorMessage, sizeof errorMessage);
            Serial.println(errorMessage);
            return;
        }
        // Provide the sensor values for Tools -> Serial Monitor or Serial
        // Plotter
        Serial.print("CO2[ppm]:");
        Serial.print(co2);
        Serial.print("\t");
        Serial.print("Temperature[℃]:");
        Serial.print(temperature);
        Serial.print("\t");
        Serial.print("Humidity[%]:");
        Serial.println(humidity);
    }
#endif
#ifdef USESCD4X
    // Read the SCD4X CO2 sensor
    error = sensor.readMeasurement(co2, temperature, humidity);
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
#endif

    // Report sensor readings via BLE
    provider.writeValueToCurrentSample(co2, SignalType::CO2_PARTS_PER_MILLION);
    provider.writeValueToCurrentSample(temperature, SignalType::TEMPERATURE_DEGREES_CELSIUS);
    provider.writeValueToCurrentSample(humidity, SignalType::RELATIVE_HUMIDITY_PERCENTAGE);
    provider.commitSample();
    provider.handleDownload();

    // Pulse blue LED
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
}

// Task callback
void readSensorCallback() {
    // Read the SCD4X CO2 sensor
    error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.print(errorMessage);
    } else if (co2 == 0) {
        Serial.print("Invalid sample detected, skipping.");
    } else {
        Serial.print((String)"Co2: " + co2);
        Serial.print((String)"Temperature: " + temperature);
        Serial.print((String)"Humidity: " + humidity);
        Serial.print("");
    }

    // Report sensor readings via BLE
    provider.writeValueToCurrentSample(co2, Unit::CO2);
    provider.writeValueToCurrentSample(temperature, Unit::T);
    provider.writeValueToCurrentSample(humidity, Unit::RH);
    provider.commitSample();
    provider.handleDownload();
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

    pinMode(LED_BUILTIN, OUTPUT);

    // Sensor setup
    Wire.begin();

    uint16_t error;
    char errorMessage[256];

#ifdef USESCD30
    // SCD30
    sensor.begin(Wire, SCD30_I2C_ADDR_61);
#endif
#ifdef USESCD4X
    // SCD4X
    sensor.begin(Wire);
#endif

    // stop potentially previously started measurement
    error = sensor.stopPeriodicMeasurement();
    if (error) {
      printToSerial("Error trying to execute stopPeriodicMeasurement(): ");
      errorToString(error, errorMessage, 256);
      printToSerial(errorMessage);
    }
#ifdef USESCD30
    // SCD30
    sensor.softReset();
    sensor.activateAutoCalibration(1);
#endif

#ifdef USESCD4X
    uint16_t serial0;
    uint16_t serial1;
    uint16_t serial2;
    error = sensor.getSerialNumber(serial0, serial1, serial2);
    if (error) {
      printToSerial("Error trying to execute getSerialNumber(): ");
      errorToString(error, errorMessage, 256);
      printToSerial(errorMessage);
    } else {
      if (Serial) {
        printSerialNumber(serial0, serial1, serial2);
      }
    }
#endif

    // Start Measurement
#ifdef USESCD30
    // SCD30
    error = sensor.startPeriodicMeasurement(0);
#endif
#ifdef USESCD4X
    // SCD4X
    error = sensor.startPeriodicMeasurement();
#endif

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
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    delay(10);

    // Set WiFi power
    // Max: WIFI_POWER_19_5dBm ~150mA
    // Min: WIFI_POWER_MINUS_1dBm ~120mA
    // WiFi.setTxPower(WIFI_POWER_2dBm);

    // We start by connecting to a WiFi network
    printToSerial((String)"Connecting to " + ssid);

    WiFi.begin(ssid, password);

    // Wait for a WiFi connection for up to 10 seconds
    for (int i = 0; i < 10; i++) {
      if (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        printToSerial(".");
        delay(500);
      } else {
        printToSerial("WiFi connected.");
        printToSerial("IP address: ");
        printToSerial((String)WiFi.localIP());

        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
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
    digitalWrite(5, HIGH);
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
            digitalWrite(LED_BUILTIN, HIGH);

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

            digitalWrite(LED_BUILTIN, LOW);

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
