#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <Arduino.h>
#include <ESP_Mail_Client.h>

// Email Services
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "teo909714@gmail.com"
#define AUTHOR_PASSWORD "knft iqdy zaoe phxl"
#define RECIPIENT_EMAIL "theo.pratnau@gmail.com"
SMTPSession smtp;

#define RST_PIN 3  // Configurable, see typical pin layout above
#define SS_PIN 4   // Configurable, see typical pin layout above

// Ultrasonic sensor
int trigPin = 0;  // Trigger
int echoPin = 2;  // Echo

long duration, cm, inches;

// RGB LED
int redPin = 15;
int greenPin = 16;

bool lastLightState = true; // Assuming the light starts in the 'green' state

FirebaseData fbdo;
FirebaseJson json;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

bool isObjThere = false; // Flag to track if object is near
unsigned long cardRemovedTime = -9999999999; // Timestamp of when the card was removed
// const unsigned long cardRemovedInterval = 900000; // 15 minutes in milliseconds
const unsigned long cardRemovedInterval = 1000*60; // 1 minutes in milliseconds

unsigned long lastRFIDCheck = 0; // Variable to store the last time RFID was checked
float rfidCheckInterval = 5000; // Timer interval in milliseconds

// States of the system
bool cardPresent = false; // Current state of card presence
bool cardPresentPrev = true; // Previous state of card presence
bool timerExpired = true; // Flag to indicate timer expiration
bool objectDetected = false; // Flag to indicate object detection
long objectDistance = 1.0;
long previousObjectDistance = 1000; // Arbitrary high value to start

String cardValue = "";

int absentCounter = 0;             // Counter for consecutive absences of the card
const int absentThreshold = 50;     // Threshold for considering the card as removed

// Replace with your network credentials
const char* ssid = "iphone5";
const char* password = "mejdimej";

// Replace with your Firebase project details
const char* FIREBASE_HOST = "siot-16d63-default-rtdb.europe-west1.firebasedatabase.app";
const char* API_KEY = "AIzaSyBajN795iPI-_xAKalgH1MEzX74z6OImbM";

// Firebase authentication
const char* USER_EMAIL = "theo.pratnau@gmail.com";
const char* USER_PASSWORD = "123456";

#define SEAT_NUMBER 2
String seatPath = "Seats/" + String(SEAT_NUMBER);

  Session_Config config;
void smtpCallback(SMTP_Status status);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  SPI.begin();
  mfrc522.PCD_Init();
  delay(4);
  mfrc522.PCD_DumpVersionToSerial();

  // Ultrasonic Sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // RGB LED
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  setColor(0, 255, 0);  // Green Color

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  smtp.callback(smtpCallback);

  // SMTP Configuration
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";

   /* Connect to the server */
  if (!smtp.connect(&config)){
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn()){
    Serial.println("\nNot yet logged in.");
  }
  else{
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }

  // Assign the project host and api key
  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.api_key = API_KEY;

  // Assign the user sign in credentials
  firebaseAuth.user.email = USER_EMAIL;
  firebaseAuth.user.password = USER_PASSWORD;

  // Initialize the library with the Firebase authentication and config.
  Firebase.begin(&firebaseConfig, &firebaseAuth);
}

String readRFID() {
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidString += mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ";
    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }
  uidString.toUpperCase(); // Convert to uppercase
  return uidString;
}

long distanceSensor() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);

  // Convert the time into a distance
  cm = (duration / 2) / 29.1;

  return cm;
}

void setColor(int redValue, int greenValue, int blueValue) {
  analogWrite(redPin, redValue);
  analogWrite(greenPin, greenValue);
}

void updateSeatInDB(bool isTaken) {
  FirebaseJson json;
  json.set("taken", isTaken); // Set the "taken" field to the provided boolean value

  if (Firebase.updateNode(fbdo, "Seats/"+String(SEAT_NUMBER)+"/Availablility", json)) {
    Serial.println("Updated seat state successfully: " + String(isTaken));
  } else {
    Serial.println("Failed to update seat state: " + String(isTaken));
    Serial.println(fbdo.errorReason());
  }
}

void pushToDatabase(String event, String cardVal = "") {
    FirebaseJson json;
    json.set("event", event);
    json.set("timestamp/.sv", "timestamp");

    // Add cardUID only if it's provided (i.e., not an empty string)
    if (cardVal != "") {
        json.set("cardUID", cardVal);
    }

    if (Firebase.pushJSON(fbdo, seatPath, json)) {
        Serial.println("Pushed JSON Successfully: " + event);
    } else {
        Serial.println("Failed to Push JSON: " + event);
        Serial.println(fbdo.errorReason());
    }
}

void updateLightStateInDB(bool lightState) {
    FirebaseJson json;
    String state = lightState ? "Green" : "Red"; // Convert boolean to string
    json.set("lightState", state); // Set the "lightState" field

    if (Firebase.updateNode(fbdo, seatPath, json)) {
        Serial.println("Updated light state successfully: " + state);
    } else {
        Serial.println("Failed to update light state: " + state);
        Serial.println(fbdo.errorReason());
    }
}

void sendEmail(const String &subject, const String &message) {
    SMTP_Message mail;
    mail.sender.name = F("ESP");
    mail.sender.email = AUTHOR_EMAIL;
    mail.subject = subject;
    mail.addRecipient(F("Recipient Name"), RECIPIENT_EMAIL);
    mail.text.content = message.c_str();
    mail.text.charSet = "us-ascii";
    mail.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    // Ensure SMTP session is connected
    if (!smtp.connect(&config)) {
        Serial.println("Connection error: " + smtp.errorReason());
        return;
    }

    // Check if logged in
    if (!smtp.isLoggedIn()) {
        Serial.println("SMTP not logged in");
        return;
    }

    if (!MailClient.sendMail(&smtp, &mail)) {
        Serial.println("Error sending Email: " + smtp.errorReason());
    }

    // Optionally, disconnect the SMTP session after sending the email
    smtp.closeSession();
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
    // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
    // In ESP8266 and ESP32, you can use Serial.printf directly.

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)
      
      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}

bool setGreen = true; // Flag to determine if the LED should be green

void loop() {

  cardValue = "";
  bool isNewCardPresent = mfrc522.PICC_IsNewCardPresent();
  bool isCardRead = mfrc522.PICC_ReadCardSerial();

  if (isNewCardPresent && isCardRead) {
      absentCounter = 0; // Reset the absent counter
      if (!cardPresent) {
          // Card added
          cardPresent = true;
          Serial.print("Card added. UID: ");
          cardValue = readRFID();
          Serial.println(cardValue);
      }
      //   Serial.println("Card still there");
      // setColor(255, 0, 0); // Red Color
      setGreen = false;
  } else {
      absentCounter++;
      if (cardPresent && absentCounter >= absentThreshold) {
          // Card removed
          cardPresent = false;
          Serial.println("Card removed.");
          cardRemovedTime = millis();
      }
  }

    if (millis() - lastRFIDCheck > rfidCheckInterval) {
        // checkRFID();
        lastRFIDCheck = millis(); // Update the last check time

        objectDistance = distanceSensor();
        isObjThere = objectDistance <= 60;

        // Check for object addition
        if (previousObjectDistance > 60 && objectDistance <= 60) {
            Serial.println("Object added.");
            pushToDatabase("Object Added");
            
            // setColor(255, 0, 0); // Red Color
            setGreen = false; // Set LED to red (or any other color to indicate object addition)
        }

        previousObjectDistance = objectDistance;

        Serial.print("Object Distance: ");
        Serial.println(objectDistance);
    }

    // Check card presence state change
    if (cardPresent != cardPresentPrev) {
        pushToDatabase(cardPresent ? "Card Placed" : "Card Removed", cardValue);
        cardPresentPrev = cardPresent;
    }

 // Handle timer expiration
    if (!cardPresent && isObjThere) {
        if (millis() - cardRemovedTime > cardRemovedInterval){
            setGreen = true; // Set LED to green
        
            if (!timerExpired) {
                timerExpired = true;
                pushToDatabase("Timer Expired");

                String emailTxt = "The timer on seat " + String(SEAT_NUMBER) + " has expired.";
                const char* emailText = emailTxt.c_str();

                sendEmail("Timer Expired", emailTxt);
            }
        } else {
            // setColor(255, 0, 0); // Red Color
            setGreen = false;
            timerExpired = false;
        }
    }

    // Set the LED color to green when there's no card and no object
    if (!cardPresent && !isObjThere) {
        setGreen = true;
        cardRemovedTime = -9999999999;

        if (objectDetected) {
            objectDetected = false;
            pushToDatabase("Object Removed");
            
            setGreen = true; // Set LED to green
        }
    } else {
        objectDetected = true;
    }


    // Set the LED color
    if (setGreen) {
        setColor(0, 255, 0); // Green Color
    } else {
        setColor(255, 0, 0); // Red Color
    }
     // Check if the light state has changed
    if (setGreen != lastLightState) {
        // Update the new state in the database
        updateLightStateInDB(setGreen);
        
        // Update the last light state
        lastLightState = setGreen;
    }
}
