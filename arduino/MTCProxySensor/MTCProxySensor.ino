// ---------------------------------------------------------------- 
//                                                                  
// MTConnect Adapter for ESP8266
//
// (c) Rolf Wuthrich, 
//     2023 Concordia University
//
// author:  Rolf Wuthrich
// email:   rolf.wuthrich@concordia.ca
//
// This software is copyright under the BSD license
//
// --------------------------------------------------------------- 

// Demonstrates how to setup an MTConnect Adapter 
// for an inductive proximity sensor LJ12A3-4-Z/BY
//
// The adapter sends SHDR format to an MTConnect Agent which connected
// to this adapter
//
// The adapter assumes the following configuration 
// in the MTConnect device model:
//
//   <DataItem category="EVENT" id="trigger" type="TRIGGER"/>
//
//
// Required boards (one of the following):
//
// - ESP8266 Arduino board
//   https://arduino-esp8266.readthedocs.io/en/3.0.2/
// - ESP32 Ardunio board 
//   https://docs.espressif.com/projects/arduino-esp32/en/latest/index.html
// 
// Available via Sketch > Include Library > Manage Libraries:


#ifdef ESP32
#include <WiFi.h>
#define LED_BUILTIN 2
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include "secrets.h"


// -----------------------------------------------------
// Configuration for WiFi access
const char *ssid     = SECRET_SSID;     // WIFI ssid
const char *password = SECRET_PASS;     // WIFI password


// -----------------------------------------------------
// Configuration for MTConnect Adapter

// Hostname
String ADAPTER_HOSTNAME = "MTCAdapter";

// Port number
const uint16_t port = 7878;

// PONG (answer to '*PING' request from the MTConnect Agent)
#define PONG "* PONG 60000"


// -----------------------------------------------------
// Configuration for proximity sensor

// Pin for 1-wire connection of DHT11
// Ref for pinout: 
// https://randomnerdtutorials.com/getting-started-with-esp8266-wifi-transceiver-review/
#define TRIGGERPIN 4              // This is D2

// Time sensors waits before re-arming the trigger (in ms)
#define DEAD_TIME 2000


// Global variables for sensor data
float triggerOld;


// ------------------------------------------------
// Global variables for adapter

WiFiServer server(port);   // max_clients = 1
WiFiClient client;
bool connected = false;


// -----------------------------------------------------------
// Adapter functions

void sendTriggerSHDR(float trigger)
{
  String shdr;
  if (trigger==LOW) {
    shdr = "|trigger|TRIGGERED";
    digitalWrite(LED_BUILTIN, LOW);
  }
  else {
    shdr = "|trigger|ARMED";
    digitalWrite(LED_BUILTIN, HIGH);
  }
  Serial.println(shdr);
  client.println(shdr);
  if (trigger==LOW) delay(DEAD_TIME);
}


void setup() {

  // initialize digital pin LED_BUILTIN as an output
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Start the Serial Monitor
  Serial.begin(115200);

  // Start proximity sensor
  digitalWrite(LED_BUILTIN, 1);
  triggerOld = 0;

  // Configure hostename
  WiFi.setHostname(ADAPTER_HOSTNAME.c_str());

  // Conencting to WiFi
  Serial.print("Connecting to WiFi ...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start TCP server (MTConnect Adapter)
  server.begin();
  Serial.println();
  Serial.println("=======================================");
  Serial.print("Starting MTConnect Adapter on port ");
  Serial.println(port);
  Serial.println("=======================================");
  Serial.println();
  Serial.println("Waiting for connection from MTConnect agent");
}

void loop() {

  if (!connected) {
    client = server.available();
    if (client) {
      if (client.connected()) {
        Serial.print("Connection recieved from ");
        Serial.println(client.remoteIP());
        Serial.println("|avail|AVAILABLE");
        client.println("|avail|AVAILABLE");
        connected = true;
      } else {    
        // the connection was not a TCP connection  
        client.stop();  // close the connection:
      }
    }
  } 
  else {
    if (client.connected()) {
      
      // collect sensor data
      float trigger = digitalRead(TRIGGERPIN);
      
      // Check for * PING request
      String currentLine = "";
      while (client.available()) { 
        char c = client.read(); 
        if (c == '\n') {
          if (currentLine.startsWith("* PING")) {
            client.println(PONG);
          }
          Serial.println(currentLine);
          currentLine = "";
        }
        currentLine += c;
      }

      // sends SHDR data
      if (trigger!=triggerOld) {
        sendTriggerSHDR(trigger);
        }
      triggerOld = trigger;

    }
    else {
      Serial.println("Client has disconnected ");
      client.stop();
      connected = false;
    }
  }

}
