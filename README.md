# Spring 2026/Fall 2026 Arduino Cutdown

This software runs on a Arduino UNO R4 WiFi connected to a Rockblock 9603 satellite module

# Libraries

- Servo
- IridiumSBD by Mikal Hart
- WiFiS3
- BlynkSimpleWifi

# Setup

1. Create Blynk device with terminal (virtual pin input)
2. Put Blynk ID and name into the define fields.
3. Put Blynk auth token, WiFi SSID, and WiFi password into a file called "secrets.h" (see the example file for reference).

Note: The device not execute any code until connected to Blynk, but once connected will run even after being disconnected

# Cutdown

- The current cutdown mechanism repeatedly actuates the servo to release payload
- Cutdown automatically occurs after 90 minutes if not already activated
- Users can also manually start the cutdown with !Acc and !Aco commands either from Blynk (during testing) or remotely with RockBLOCK
  - website to send cutdown commands: https://rockblock.rock7.com/Operations

# Rockblock 9603 information

We are using the RockBLOCK 9603 model. The model comes with an external antenna that is screwed onto the RockBLOCK itself.

PINOUT, Arduino to RockBLOCK

1. RX -> RXD
2. TX -> TXD
3. 5V -> VCC
4. GND -> GND

##### Overview of code used to interface with the rockblock:

#TBD

##### Overview on rockblock website for sending commands: 

#TBD, add pictures
