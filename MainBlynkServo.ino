// this code works with blynk with the uno r4, can read what is being printed on serial monitor 


// Paste in your Blynk information from the dashboard
#define BLYNK_TEMPLATE_ID "TMPL2tSBE42IL"
#define BLYNK_TEMPLATE_NAME "Rockblock Terminal"
#define BLYNK_AUTH_TOKEN "Y6jcLMKWOxE5FCyg30M7h6l0XzSPwfuR"
#include <Wire.h>              //OLED/LUX
#include <SPI.h>               //SD/ATM
#include <SD.h>                //SD
#include <TinyGPS++.h>         //GPS          TinyGPSPlus by Mikal Hart
#include <Adafruit_SSD1306.h>  //OLED         Adafruit SSD1306 by Adafruit
#include <Adafruit_Sensor.h>   //ATM/LUX
#include <Adafruit_BME680.h>   //ATM          Adafruit BME680 Library by Adafruit
#include "Adafruit_TSL2591.h"  //LUX          Adafruit TSL2591 Library by Adafruit
#include "Adafruit_SHT31.h"    // TEMP/HU     Adafruit SHT31 Library by Adafruit

#include <Servo.h>

#include <IridiumSBD.h>        //             Iridium SBD by Mikal Hart
#include <WiFiS3.h>
#include <BlynkSimpleWifi.h>



#define DIAGNOSTICS false  // Change this to see diagnostics

//=====================================
//ATM =================================
#define SEALEVELPRESSURE_HPA (1013.2)
#define BME_SCK 40
#define BME_MISO 41
#define BME_MOSI 42
#define BME_CS 43
#define seconds() (millis() / 1000)

Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);

#define IridiumSerial Serial1 // The arduino mega has multiple uart ports (tx/rx1-3), the uno r4 only has Serial1

//Rockblock variables =======================
IridiumSBD iridium(IridiumSerial);

uint8_t buffer[200] = { 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89 };

int buzzerPin = 11;

int sq;

bool sendSuccess;

bool cut;

uint8_t bufferSend[270];
size_t bufferSizeSend = 0;

//=====================================

//ATM variables =======================
float pascals;
float altm;
float humidity;
float gas;
float tempC;

//=====================================
//LUX =================================
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);

//LUX variables =======================
uint32_t lum;
int16_t ir, full;
float lux;

TinyGPSPlus gps;
double latit;   //degrees
double longit;  //degrees
double alt;     //meters
double alto;
double velDiff;
double vel[] = { 0, 0, 0, 0, 0 };  //meters
long gpstime;                      //HHMMSSCC
long gpsdate;                      //DDMMYY
int numsats;
double course;  //degrees
double speed;   //mph

//SD Card ==================================
#define chipSelect 53

//TEMP/HU ==================================
Adafruit_SHT31 sht31 = Adafruit_SHT31();
double sht31_temp;
double sht31_humidity;

//Servo ====================================
Servo myServo;

char ssid[] = "iPhone Chris Wang"; // if using an iOS hotspot, turn on "Maximize Compatibility"
char pass[] = "crtqveus";

BLYNK_WRITE(V0) {
  String receivedString = param.asString();
    
  Serial.println("Received from Blynk Terminal: " + receivedString); 

  if (receivedString.equals("!Acc")) {
    myServo.write(75);                                             //turn servo
    Blynk.virtualWrite(V0, "Release message sent to cutter!");
  } else if (receivedString.equals("!Aco")) {
    myServo.write(0);                                             //turn servo
    Blynk.virtualWrite(V0, "Retract message sent to cutter!");
  } else {
    Blynk.virtualWrite(V0, "Other message sent to cutter!");
  }
}

void setup() {
  //Serial
  Serial.begin(115200);                       //Computer

  Serial.println("Test"); 
  //Serial1.begin(115200);                      //cutter uart, change wired connection
  //Serial2.begin(9600);                        //GPS uart

  myServo.attach(3);                          
  myServo.write(0);                           //starting servo position

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);  //blynk terminal

  //rockblock
  pinMode(buzzerPin, OUTPUT);
  while (!Serial);
  IridiumSerial.begin(19200);

  // Setup the Iridium modem
  Serial.println("Hello! Checking signal quality...");
  Blynk.virtualWrite(V0, "Hello! Checking signal quality...");
  iridium.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);
  if (iridium.begin() != ISBD_SUCCESS) {
    Serial.println("Couldn't begin modem operations.");
    Blynk.virtualWrite(V0, "Couldn't begin modem operations.");
    exit(0);
  }
  //rockblock
  cut = false;
}

static bool messageSent = false;

void loop() {
  
  Blynk.run();

  if (seconds() > 5400 && cut == false) {  // cutdown after 90min
    for (int i = 0; i < 5; i++) {
      //Serial1.write("!Acc");  // serial1 is 19 (RX1), 18 (TX1), REPLACED WITH Iridium
      myServo.write(90); 
      delay(3000);
      //Serial1.write("!Aco");
      delay(3000);
    }
    //Serial1.write("!Acc");
    cut = true;
  }
  // String data = mkdata();
  // Serial.println(data);
  // writeSD(data);
  // delay(100);

  // String dataSend = mkdataSend();
  // char outBuffer[270];
  // for (int i = 0; i < dataSend.length(); i++) {
  //   outBuffer[i] = dataSend[i];
  // }

  // uint8_t rxBuffer[270] = {0};
  // for (int i = 0; i < dataSend.length(); i++) {
  //   rxBuffer[i] = outBuffer[i];
  // }

  bufferSizeSend = sizeof(bufferSend);

  int signalQuality = -1;
  // Check the signal quality
  int errS = iridium.getSignalQuality(signalQuality);
  if (errS != 0) {
    Serial.print("SignalQuality failed: error ");
    Blynk.virtualWrite(V0, "SignalQuality failed: error ");
    Serial.println(errS);
    Blynk.virtualWrite(V0, errS);
    exit(1);
  }
  Serial.print("Signal quality is ");
  Blynk.virtualWrite(V0, "Signal quality is ");
  Serial.println(signalQuality);
  Blynk.virtualWrite(V0, signalQuality);
  sq = signalQuality;

  //indentation?????
  // Control the active buzzer to beep 'x' times in a group

  int pitches[] = { 440, 667, 880, 1333, 1011, 880 };

  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < sq + 1; i++) {
      // Beep (generate a tone) for a specific duration
      beep(pitches[i], 500);
      delay(100);
    }
    // Add a longer delay between groups if needed
  }

  if (sq > 0) {
    delay(1000);
    int err;
    Serial.println("Waiting for messages...");
    Blynk.virtualWrite(V0, "Waiting for messages...");
    beep(581, 2000);
    // Read/Write the first time or if there are any remaining messages
    if (!messageSent || iridium.getWaitingMessageCount() > 0) {
      size_t bufferSize = sizeof(buffer);

      if (!messageSent)
        err = iridium.sendReceiveSBDBinary(buffer, 11, buffer, bufferSize);
      else
        err = iridium.sendReceiveSBDText(NULL, buffer, bufferSize);
      Serial.println(iridium.getWaitingMessageCount());
      Blynk.virtualWrite(V0, iridium.getWaitingMessageCount());

      if (err != ISBD_SUCCESS) {
        Serial.print("sendReceiveSBD* failed: error ");
        beep(506, 250);
        beep(440, 250);
        beep(383, 250);
        beep(506, 250);
        beep(440, 250);
        beep(383, 250);
        Serial.print("Timeout after 30 seconds...Probably due to bad signal quality. Error code: ");
        Blynk.virtualWrite(V0, "Timeout after 30 seconds...Probably due to bad signal quality. Error code: ");
        Serial.println(err);
        Blynk.virtualWrite(V0, err);
        Serial.println("Checking signal quality again...");
        Blynk.virtualWrite(V0, "Checking signal quality again...");
      } else if (iridium.getWaitingMessageCount() == 0) {
        Serial.println("No Messages to Display.");
        Blynk.virtualWrite(V0, "No Messages to Display.");
      } else  // success!
      {
        messageSent = true;
        Serial.println();
        Serial.print("Inbound buffer size is ");
        Blynk.virtualWrite(V0, "Inbound buffer size is ");
        Serial.println(bufferSize);
        Blynk.virtualWrite(V0, bufferSize);
        char mes[strlen((const char*)buffer) + 1];
        for (int i = 0; i < bufferSize; ++i) {
          Serial.print(buffer[i], HEX);
          if (isprint(buffer[i])) {

            Serial.print("(");
            Serial.write(buffer[i]);
            Serial.print(")");
          }
          mes[i] = buffer[i];
        }
        mes[strlen((const char*)buffer)] = '\0';
        Serial.println();
        Serial.print("The full message received is: ");
        Blynk.virtualWrite(V0, "The full message received is: ");
        Serial.println(mes);
        Blynk.virtualWrite(V0, mes);
        //       //change corresponding wiring
        //Serial1.write(mes); NOTHING IS BEING WRITTEN TO THE CUTTER
        beep(300, 500);
        beep(400, 500);
        beep(500, 500);
        beep(600, 500);
        beep(800, 500);
        Serial.println();
        Serial.print("Messages sent to cutter!");

        myServo.write(180); //testing purposes
        if ((String(mes)).equals("!AccoQ")) {
          myServo.write(75);                                             //turn servo
        } else if ((String(mes)).equals("!AcooQ")) {
          myServo.write(0);
        }
        
        Blynk.virtualWrite(V0, "Messages sent to cutter!");
        Serial.println();
        Serial.print("Messages remaining to be retrieved: ");
        Blynk.virtualWrite(V0, "Messages remaining to be retrieved: ");
        Serial.println(iridium.getWaitingMessageCount());
        Blynk.virtualWrite(V0, iridium.getWaitingMessageCount());

        // int senderr;
        // Serial.println("Sending data to control...");
        // beep(1000,2000);
        // senderr = modem.sendReceiveSBDBinary(rxBuffer, dataSend.length(), bufferSend, bufferSizeSend);
        // if (senderr != ISBD_SUCCESS)
        // {
        //        beep(3700, 1000);
        //        beep(3100, 1000);
        //        beep(2500, 1000);
        //        Serial.print("Unable to send data to control after 30 seconds...Probably due to bad signal quality. Error code: ");
        //        Serial.println(senderr);
        //        sendSuccess = false;
        //  }
        //  else{
        //    Serial.print("Message successfully sent to control is: ");
        //    Serial.println(dataSend);
        //    sendSuccess = true;
        //  }
      }
    }
  }
}
// Function to generate a beep with a specific frequency and duration
void beep(int frequency, int duration) {
  tone(buzzerPin, frequency);
  delay(duration);
  noTone(buzzerPin);
}

#if DIAGNOSTICS
void ISBDConsoleCallback(IridiumSBD* device, char c) {
  Serial.write(c);
}

void ISBDDiagsCallback(IridiumSBD* device, char c) {
  Serial.write(c);
}
#endif
