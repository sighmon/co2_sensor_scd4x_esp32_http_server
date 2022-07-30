/*
 Arduino ESP32 WiFi Web Server for the Adafruit SCD30 CO2 sensor.
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
 * Adafruit SCD30 test Copyright (c) 2020 Bryan Siepert for Adafruit Industries

 Written by Simon Loffler on invasion/survival day 26/1/2022
*/

#include "secrets.h"

// SCD4X sensor init

#include <Arduino.h>
#include <Adafruit_SCD30.h>

Adafruit_SCD30  scd30;
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

    // SCD30 setup
    if (!scd30.begin()) {
      Serial.println("Failed to find SCD30 chip");
      while (1) { delay(10); }
    }
    Serial.println("SCD30 Found!");

    Serial.print("Measurement Interval: ");
    Serial.print(scd30.getMeasurementInterval());
    Serial.println(" seconds");

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
            client.println("Content-type:text/html; charset=UTF-8");
            client.println();

            // Read the SCD30 CO2 sensor
            digitalWrite(5, HIGH);
            if (scd30.dataReady()){
              if (!scd30.read()){ Serial.println("Error reading sensor data"); return; }
              co2 = scd30.CO2;
              temperature = scd30.temperature;
              humidity = scd30.relative_humidity;
            }
            Serial.print("Co2:");
            Serial.println(co2);
            Serial.print("Temperature:");
            Serial.println(temperature);
            Serial.print("Humidity:");
            Serial.println(humidity);
            Serial.println();

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
            digitalWrite(5, LOW);
            // END Read the SCD30 CO2 sensor

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
    Serial.println("Client Disconnected.");
  }
}
