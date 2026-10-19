// this code works with blynk with the uno r4, can read what is being printed on serial monitor 

// Public Blynk device identifiers, private credentials (Blynk auth and hotspot login) go in secrets.h.
#define BLYNK_TEMPLATE_ID "TMPL2tSBE42IL"
#define BLYNK_TEMPLATE_NAME "Rockblock Terminal"

#include "secrets.h" // hotspot login

#include <Servo.h>
#include <IridiumSBD.h>
#include <WiFiS3.h>
#include <BlynkSimpleWifi.h>

#define seconds() (millis() / 1000)

// Servo: digital pin 3 (pwm), 3.3V pin
Servo myServo;
const int SERVO_PIN = 3;
const int ANGLE_START = 0;
const int ANGLE_OPEN = 63;

const unsigned long BLYNK_RETRY_MS = 60000; // if Blynk isn't connected, retry this often
unsigned long lastBlynkAttempt = 0;

// Rockblock variables 
// the pins are 0-RX to RXD, 1-TX to TXD, 5V to Vcc, GND to GND
#define IridiumSerial Serial1 // The arduino mega has multiple uart ports (tx/rx1-3), the uno r4 only has Serial1
IridiumSBD iridium(IridiumSerial);
uint8_t buffer[200] = { 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89 };
static bool messageSent = false;
bool cut;

// Blynk.begin() without its infinite retry loop: join WiFi, then the Blynk server, each giving up after timeoutMs
void connectBlynk(unsigned long timeoutMs) {
  lastBlynkAttempt = millis();
  WiFi.setTimeout(timeoutMs);
  if (WiFi.status() != WL_CONNECTED && WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.println("WiFi not connected");
  } else if (Blynk.connect(timeoutMs)) {
    Serial.println("Blynk connected");
    Blynk.virtualWrite(V0, "Blynk connected");
  } else {
    Serial.println("Blynk server not connected");
  }
}

// Run Blynk while connected, otherwise retry every BLYNK_RETRY_MS
void serviceBlynk() {
  if (Blynk.connected()) {
    Blynk.run();
  } else if (millis() - lastBlynkAttempt >= BLYNK_RETRY_MS) {
    connectBlynk(5000); // short, so RockBLOCK checks aren't held up
  }
}

// Writing from the blynk to Arduino, for testing the cutter
BLYNK_WRITE(V0) {
  String receivedString = param.asString();
    
  Serial.println("Received from Blynk Terminal: " + receivedString); 

  if (receivedString.equals("!Acc")) {
    myServo.write(ANGLE_OPEN);  //turn servo
    Blynk.virtualWrite(V0, "Release message sent to cutter!");
  } else if (receivedString.equals("!Aco")) {
    myServo.write(ANGLE_START);  //turn servo
    Blynk.virtualWrite(V0, "Retract message sent to cutter!");
  } else {
    Blynk.virtualWrite(V0, "Other message sent to cutter!");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  Serial.println("Test"); 

  myServo.attach(3);                          
  myServo.write(ANGLE_START);  // starting servo position

  Blynk.config(BLYNK_AUTH_TOKEN);
  connectBlynk(20000); // replaces Blynk.begin(), which never returns without a connection

  // Rockblock setup
  IridiumSerial.begin(19200);

  // Setup the Iridium modem
  Serial.println("Hello! Checking signal quality...");
  Blynk.virtualWrite(V0, "Hello! Checking signal quality...");
  iridium.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);
  if (iridium.begin() != ISBD_SUCCESS) {
    Serial.println("Couldn't begin modem operations");
    Blynk.virtualWrite(V0, "Couldn't begin modem operations");
  } else {
    Serial.println("RockBLOCK initialized");
  }
  cut = false;
}

void loop() {
  
  serviceBlynk();

  if (seconds() > 5400 && cut == false) {  // automatic cutdown after 90min
    for (int i = 0; i < 5; i++) {
      myServo.write(ANGLE_OPEN); 
      delay(6000);
    }
    cut = true;
  }

  int signalQuality = -1;
  // Check the signal quality (ranging from 1 to 5), return it if connection is made
  int errS = iridium.getSignalQuality(signalQuality);
  if (errS != 0) {
    Serial.print("SignalQuality failed: error ");
    Blynk.virtualWrite(V0, "SignalQuality failed: error ");
    Serial.println(errS);
    Blynk.virtualWrite(V0, errS);
  }
  Serial.print("Signal quality is ");
  Blynk.virtualWrite(V0, "Signal quality is ");
  Serial.println(signalQuality);
  Blynk.virtualWrite(V0, signalQuality);

  if (signalQuality > 0) {
    delay(1000);
    int err;
    Serial.println("Waiting for messages...");
    Blynk.virtualWrite(V0, "Waiting for messages...");
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

        Serial.print("Timeout after 30 seconds...Probably due to bad signal quality. Error code: ");
        Blynk.virtualWrite(V0, "Timeout after 30 seconds...Probably due to bad signal quality. Error code: ");
        Serial.println(err);
        Blynk.virtualWrite(V0, err);
        Serial.println("Checking signal quality again...");
        Blynk.virtualWrite(V0, "Checking signal quality again...");
      } else if (iridium.getWaitingMessageCount() == 0) {
        Serial.println("No Messages to Display.");
        Blynk.virtualWrite(V0, "No Messages to Display.");
      } else {
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
        
        Serial.println();
        Serial.print("Messages sent to cutter!");

        // servo control
        myServo.write(ANGLE_START); 
        if (strncmp(mes, "!Acc", 4) == 0) {
          Serial.println("!Acc");
          myServo.write(ANGLE_OPEN);          
        } else if (strncmp(mes, "!Aco", 4) == 0) {
          Serial.println("!Aco");
          myServo.write(ANGLE_START);
        } else {
          Serial.println("Unknown command");
        }
        
        Blynk.virtualWrite(V0, "Messages sent to cutter!");
        Serial.println();
        Serial.print("Messages remaining to be retrieved: ");
        Blynk.virtualWrite(V0, "Messages remaining to be retrieved: ");
        Serial.println(iridium.getWaitingMessageCount());
        Blynk.virtualWrite(V0, iridium.getWaitingMessageCount());
      }
    }
  }
}
