// this code works with blynk with the uno r4, can read what is being printed on serial monitor 

// Paste in your Blynk information from the website dashboard
#define BLYNK_TEMPLATE_ID "TMPL2tSBE42IL"
#define BLYNK_TEMPLATE_NAME "Rockblock Terminal"
#define BLYNK_AUTH_TOKEN "Y6jcLMKWOxE5FCyg30M7h6l0XzSPwfuR"

#include <Servo.h>
#include <IridiumSBD.h>
#include <WiFiS3.h>
#include <BlynkSimpleWifi.h>

#define DIAGNOSTICS false  // Change to true to see diagnostics
#define seconds() (millis() / 1000)

// Rockblock variables 
#define IridiumSerial Serial1 // The arduino mega has multiple uart ports (tx/rx1-3), the uno r4 only has Serial1
IridiumSBD iridium(IridiumSerial);

uint8_t buffer[200] = { 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89 };
uint8_t bufferSend[270];
size_t bufferSizeSend = 0;

int buzzerPin = 11;

int sq; // Signal quality
static bool messageSent = false;

bool cut;

//Servo
Servo myServo;

// WiFi ssid and password
char ssid[] = ""; // if using an iOS hotspot, turn on "Maximize Compatibility"
char pass[] = "";

// Writing from the blynk to Arduino, for testing the cutter
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
  Serial.begin(115200);                       // Computer serial
  Serial.println("Test"); 

  myServo.attach(3);                          
  myServo.write(0);                           // starting servo position

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);  // blynk terminal setup, further code will not run until connection is made

  // Rockblock setup
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
  cut = false;
}

void loop() {
  
  Blynk.run();

  if (seconds() > 5400 && cut == false) {  // cutdown after 90min
    for (int i = 0; i < 5; i++) {
      myServo.write(90); 
      delay(3000);
      delay(3000);
    }
    cut = true;
  }

  bufferSizeSend = sizeof(bufferSend);

  int signalQuality = -1;
  // Check the signal quality (ranging from 1 to 5), return it if connection is made
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

  // more beeps signify better signal quality
  int pitches[] = { 440, 667, 880, 1333, 1011, 880 };
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < sq + 1; i++) {
      beep(pitches[i], 500);
      delay(100);
    }
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
        
        beep(300, 500);
        beep(400, 500);
        beep(500, 500);
        beep(600, 500);
        beep(800, 500);
        Serial.println();
        Serial.print("Messages sent to cutter!");

        // servo control, change the write calls to properly cutdown
        myServo.write(180); 
        if ((String(mes)).equals("!AccoQ")) {
          myServo.write(75);                                             
        } else if ((String(mes)).equals("!AcooQ")) {
          myServo.write(0);
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

// Generate a beep with a specific frequency and duration
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
