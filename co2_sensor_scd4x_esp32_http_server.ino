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

 Based on:
 * ESP32 example code SimpleWiFiServer by Jan Hendrik Berlin
 * Sensirion I2C SCD4X example code exampleUsage Copyright (c) 2021, Sensirion AG

 Written by Simon Loffler on invasion/survival day 26/1/2022
*/

#include "secrets.h"

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

// WiFi init

#include <WiFi.h>

const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASSWORD;

WiFiServer server(80);

void setup() {

    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }

    // SCD4X setup

    Wire.begin();

    uint16_t error;
    char errorMessage[256];

    scd4x.begin(Wire);

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    uint16_t serial0;
    uint16_t serial1;
    uint16_t serial2;
    error = scd4x.getSerialNumber(serial0, serial1, serial2);
    if (error) {
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        printSerialNumber(serial0, serial1, serial2);
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    Serial.println("Waiting for first measurement... (5 sec)");

    // WiFi setup
    
    pinMode(5, OUTPUT);      // set the LED pin mode

    delay(10);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    server.begin();

}

int value = 0;

void loop() {

  // WiFi server
  
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port    
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // Read the SCD4X CO2 sensor
            digitalWrite(5, HIGH);
            error = scd4x.readMeasurement(co2, temperature, humidity);
            if (error) {
                Serial.print("Error trying to execute readMeasurement(): ");
                errorToString(error, errorMessage, 256);
                Serial.println(errorMessage);
            } else if (co2 == 0) {
                Serial.println("Invalid sample detected, skipping.");
            } else {
                Serial.print("Co2:");
                Serial.println(co2);
                Serial.print("Temperature:");
                Serial.println(temperature);
                Serial.print("Humidity:");
                Serial.println(humidity);
                Serial.println();
            }
            // Send Prometheus data
            client.println("# HELP ambient_temperature Ambient temperature");
            client.println("# TYPE ambient_temperature gauge");
            client.println((String)"ambient_temperature " + temperature);
            client.println("# HELP ambient_humidity Ambient humidity");
            client.println("# TYPE ambient_humidity gauge");
            client.println((String)"ambient_humidity " + humidity);
            client.println("# HELP co2 CO2");
            client.println("# TYPE co2 gauge");
            client.println((String)"co2 " + co2);
            digitalWrite(5, LOW);
            // END Read the SCD4X CO2 sensor

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(5, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(5, LOW);                // GET /L turns the LED off
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}
