# Spring 2026 Arduino Cutdown

This software runs on a Arduino UNO R4 WiFi and Rockblock 9603

# Libraries

- Servo
- IridiumSBD by Mikal Hart
- WiFiS3
- BlynkSimpleWifi

# Setup

1. Create Blynk device with terminal (virtual pin input)
2. Put ID and auth token into the define fields.
3. Put WiFi/hotspot ssid and password into the variables

Note: The device not execute any code until connected to Blynk, but once connected will run even after being disconnected

# Cutdown

- Cutdown just repeatedly actuates the servo to release payload
- Cutdown automatically occurs after 90 minutes if not already activated
- Can manually cutdown with !Acc and !Aco commands either from Blynk (for testing) or RockBLOCK
  - website to enter this information: https://rockblock.rock7.com/Operations

# Rockblock 9603 information

We are using the Rockblock 9603 model. The model comes with an external antenna object that is screwed onto the rockblock itself. 

PINOUT, arduino to rockblock 

1. RX -> RXD
2. TX -> TXD
3. 5V -> VCC
4. GND -> GND

##### Overview of code used to interface with the rockblock:
