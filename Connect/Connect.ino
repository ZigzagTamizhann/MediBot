#include <WiFi.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// WiFi credentials
// const char* ssid = "AVS";
// const char* password = "admin@12345";

// const char* ssid = "Zigzag Network";
// const char* password = "123456789";

// const char* ssid = "KARTHI BSNL";
// const char* password = "17111971";

// const char* ssid = "VENKAT";
// const char* password = "JAYANTHI";

const char* ssid = "Zigzag Network";
const char* password = "ZIGZAG VENKAT";

// Create a server on port 80
WiFiServer server(80);

// Pin definitions
#define LED_PIN 18
#define LED_PIN1 19
#define DHT_PIN 15
#define DHT_TYPE DHT11
#define HEART_SENSOR_PIN 34
#define OLED_RESET -1

const int IN1 = 4;
const int IN2 = 2;
const int IN3 = 13;
const int IN4 = 14;

Servo myServo;         // Create a Servo object
int servoPin = 23;

// DHT sensor instance
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET); // OLED display instance

// Variables for heart rate detection
unsigned long previousMillisGetHR = 0;
unsigned long previousMillisResultHR = 0;
const long intervalGetHR = 20; // Interval for reading heart rate (20 ms)
const long intervalResultHR = 10000; // Interval for calculating BPM (10 seconds)
int PulseSensorSignal;
int UpperThreshold = 2200; // Set a higher threshold
int LowerThreshold = 2000; // Set a lower threshold
int cntHB = 0; // Counter for heartbeats
boolean ThresholdStat = true; // Flag to track state for heartbeat detection
int BPMval = 0; // Variable to store calculated BPM

// Define SIM800C module pins for ESP32
const int SIM800C_RX_PIN = 16;  // Connect to SIM800C TX
const int SIM800C_TX_PIN = 17;  // Connect to SIM800C RX

// Phone number to call and message recipient
String phoneNumber = "+919442879062";
String smsMessage = "Emergency from ICU - First FLoor";
String sms = "Patient Ok";

// Graph variables
int x = 0;
int y = 0;
int lastx = 0;
int lasty = 0;

// Heart icon bitmap, 16x16 pixels
const unsigned char Heart_Icon[] PROGMEM = {
  0x00, 0x00, 0x18, 0x30, 0x3c, 0x78, 0x7e, 0xfc, 0xff, 0xfe, 0xff, 0xfe, 0xee, 0xee, 0xd5, 0x56,
  0x7b, 0xbc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f, 0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00
};

void setup() {
  
  Serial.begin(115200);
  dht.begin();

  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_PIN1, OUTPUT);
  pinMode(HEART_SENSOR_PIN, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  myServo.attach(servoPin);  // Attach the servo to the defined pin

  // Initialize SIM800C communication
  Serial2.begin(9600, SERIAL_8N1, SIM800C_RX_PIN, SIM800C_TX_PIN);

  // Initialize the OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  for(int i=0;i<5;i++)
  {
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(1000);
  }
  
  myServo.write(90);         // Start at 90 degrees (neutral position)
  delay(1000);

  // Start the server
  server.begin();
  delay(3000);

}

void loop() {

  // Check if a client has connected
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New Client Connected.");
    String request = "";

    // Read client request
    while (client.connected()) {
      if (client.available()) {
        String request = client.readStringUntil('\n');  // Read the incoming command
        request.trim();
        Serial.print("Received Request: ");
        Serial.println(request);
        if (request.indexOf("11") >= 0) {
          digitalWrite(LED_PIN, HIGH);
          client.println("1");
          Serial.println("LED ON");
        } else if (request.indexOf("00") >= 0) {
          digitalWrite(LED_PIN, LOW);
          client.println("1");
          Serial.println("LED OFF");
        } else if (request.indexOf("Temp") >= 0) {
          detectTemperature(client);
        } else if (request.indexOf("Heart") >= 0) {
          detectHeartRate(client);
        } else if (request.indexOf("Call") >= 0) {
          sendSMS();
          delay(5000);
          makeCall();
          // Wait for 30 seconds (10000 milliseconds)
          delay(30000);
          // End the call
          endCall();
          // Send response
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.println();
          client.println("Completed");
          Serial.println("SMS sent and call made");
        } else if (request.indexOf("Call1") >= 0) {
          sendSMS1();
          delay(5000);
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.println();
          client.println("Completed");
          Serial.println("SMS made");
        } else if (request.indexOf("Too Normal") >= 0 || request.indexOf("Not Good") >= 0 || request.indexOf("Need") >= 0 || request.indexOf("BPM") >= 0 || request.indexOf("Heat") >= 0)  {      
          
          // Set SMS mode to text
          Serial2.print("AT+CMGF=1\r");
          delay(1000);

          // Specify the recipient phone number
          Serial2.print("AT+CMGS=\"" + phoneNumber + "\"\r");
          delay(1000);

          // Send the SMS content
          Serial2.print(request);
          Serial2.write(26); // Ctrl+Z character to send the message
          delay(1000);

          Serial.println("SMS sent to " + phoneNumber);

          delay(5000);
          
          String command = "ATD" + phoneNumber + ";\r";
          Serial2.print(command);
          Serial.println("Calling " + phoneNumber);
          
          // Wait for 30 seconds (10000 milliseconds)
          delay(30000); 

          endCall();
          client.println("1");
          Serial.println("Emergency SMS sent and call made");

        } else if (request.indexOf("S_1") >= 0) {
          myServo.write(90);    // Move servo to current angle
          // Perform the sequence
          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(3000);
          
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);

          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, HIGH);
          delay(1250);

          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);

          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(3000);
          
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);

          digitalWrite(IN1, LOW);
          digitalWrite(IN2, HIGH);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(1250);

          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);

          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(2000);
          
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);

          myServo.write(0);    // Move servo to current angle
          
          client.println("1");
          Serial.println("1");
          
        } else if (request.indexOf("F_2") >= 0 || request.indexOf("S_3") >= 0 || request.indexOf("T_4") >= 0 || request.indexOf("F_5") >= 0 || request.indexOf("S_7") >= 0 || request.indexOf("S_8") >= 0 ||  request.indexOf("E_9") >= 0 || request.indexOf("N_10") >= 0) {
    
          myServo.write(90);    // Move servo to current angle
          
          // Your code here
          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(2000);
          
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);
          
          myServo.write(0);    // Move servo to current angle
          
          client.println("1");
          Serial.println("1");

        } else if (request.indexOf("F_6") >= 0 ) {

          myServo.write(90);    // Move servo to current angle
          
          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(2000);
          
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);

          digitalWrite(IN1, LOW);
          digitalWrite(IN2, HIGH);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(1250);

          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);

          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(6000);
          
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);

          digitalWrite(IN1, LOW);
          digitalWrite(IN2, HIGH);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(1250);
          
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);
          
          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(2000);
          
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);

          myServo.write(0);    // Move servo to current angle
          
          client.println("1");
          Serial.println("1");
          
        } else if (request.indexOf("T_E") >= 0 ) {

          myServo.write(90);    // Move servo to current angle
          
          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(2000);
          
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);

          digitalWrite(IN1, LOW);
          digitalWrite(IN2, HIGH);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(1250);

          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);

          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(3000);
          
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);

          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, HIGH);
          delay(1250);

          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);

          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(3000);
          
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);

          digitalWrite(IN1, LOW);
          digitalWrite(IN2, HIGH);
          digitalWrite(IN3, HIGH);
          digitalWrite(IN4, LOW);
          delay(2500);

          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          digitalWrite(IN3, LOW);
          digitalWrite(IN4, LOW);
          delay(1000);
          
          myServo.write(0);    // Move servo to current angle
          
          client.println("1");
          Serial.println("1");
          
        } else {
          client.println("Invalid command");
          Serial.println("Invalid command received");
        } 
      }
    }
    client.stop();
    Serial.println("Client Disconnected.");   
  }
}

// Temperature detection function
void detectTemperature(WiFiClient& client) {
  Serial.println("Temperature detection started for 30 seconds.");
  unsigned long startTime = millis();
  while (millis() - startTime < 30000) {
    float temperature = dht.readTemperature();
    if (isnan(temperature)) {
      client.println("Failed to read temperature!");
      Serial.println("Failed to read temperature!");
    } else {
      client.printf("Temperature: %.2f\n", temperature);
      Serial.printf("Temperature: %.2f C\n", temperature);

      // Display temperature on the OLED
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextSize(1.5);
      display.print("TEMPERATURE");
      display.setCursor(0, 40);
      display.setTextSize(2);
      display.print(temperature, 2);
      display.print(" C");
      display.display();
    }
    delay(2000);
  }
  client.println("Temperature Completed");
  Serial.println("Temperature detection finished.");
}
// Heart rate detection function
void detectHeartRate(WiFiClient& client) {
  Serial.println("Heart rate detection started for 10 times.");
  for (int i = 0; i < 10; i++) { // Perform heart rate detection 10 times
    cntHB = 0; // Reset the heartbeat count for each detection
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) { // Perform detection for 10 seconds
      unsigned long currentMillisGetHR = millis();

      if (currentMillisGetHR - previousMillisGetHR >= intervalGetHR) {
        previousMillisGetHR = currentMillisGetHR;

        PulseSensorSignal = analogRead(HEART_SENSOR_PIN);

        if (PulseSensorSignal > UpperThreshold && ThresholdStat) {
          cntHB++;
          ThresholdStat = false;
          digitalWrite(LED_PIN, HIGH);
        }

        if (PulseSensorSignal < LowerThreshold) {
          ThresholdStat = true;
          digitalWrite(LED_PIN, LOW);
        }

        DrawGraph();
      }

      unsigned long currentMillisResultHR = millis();

      if (currentMillisResultHR - previousMillisResultHR >= intervalResultHR) {
        previousMillisResultHR = currentMillisResultHR;

        BPMval = cntHB * 6;
        Serial.print("BPM: ");
        Serial.println(BPMval);

        display.fillRect(20, 48, 108, 18, BLACK);
        display.drawBitmap(0, 47, Heart_Icon, 16, 16, WHITE);
        display.drawLine(0, 43, 127, 43, WHITE);
        display.setTextSize(2);
        display.setCursor(20, 48);
        display.print(": ");
        display.print(BPMval);
        display.print(" ");
        display.display();

        cntHB = 0;
        
        client.printf("BPM : %d\n", BPMval);
        Serial.printf("BPM for detection %d: %d\n", i + 1, BPMval);
      }
    }

    delay(2000); // Optional delay between each detection (2 seconds)
  }

  client.println("Heart Completed");
  Serial.println("Heart rate detection finished.");
}

void DrawGraph() {
  if (x > 127) {
    display.fillRect(0, 0, 128, 42, BLACK);
    x = 0;
    lastx = 0;
  }

  int ySignal = PulseSensorSignal;

  if (ySignal > 3500) ySignal = 3500;
  if (ySignal < 1000) ySignal = 1000;

  int ySignalMap = map(ySignal, 1000, 3500, 0, 40);

  y = 40 - ySignalMap;

  display.writeLine(lastx, lasty, x, y, WHITE);
  display.display();

  lastx = x;
  lasty = y;
  x++;
}

void makeCall() {
  String command = "ATD" + phoneNumber + ";\r";
  Serial2.print(command);
  Serial.println("Calling " + phoneNumber);
}

void endCall() {
  Serial2.print("ATH\r");
  Serial.println("Call ended");
}

void sendSMS1() {
  // Set SMS mode to text
  Serial2.print("AT+CMGF=1\r");
  delay(1000);

  // Specify the recipient phone number
  Serial2.print("AT+CMGS=\"" + phoneNumber + "\"\r");
  delay(1000);

  // Send the SMS content
  Serial2.print(sms);
  Serial2.write(26); // Ctrl+Z character to send the message
  delay(1000);

  Serial.println("SMS sent to " + phoneNumber);
}

// void sendSMS2() {

//   // Set SMS mode to text
//   Serial2.print("AT+CMGF=1\r");
//   delay(1000);

//   // Specify the recipient phone number
//   Serial2.print("AT+CMGS=\"" + phoneNumber + "\"\r");
//   delay(1000);

//   // Send the SMS content
//   Serial2.print(sms);
//   Serial2.write(26); // Ctrl+Z character to send the message
//   delay(1000);

//   Serial.println("SMS sent to " + phoneNumber);

  
//   String command = "ATD" + phoneNumber + ";\r";
//   Serial2.print(command);
//   Serial.println("Calling " + phoneNumber);
  
// }

void sendSMS() {
  // Set SMS mode to text
  Serial2.print("AT+CMGF=1\r");
  delay(1000);

  // Specify the recipient phone number
  Serial2.print("AT+CMGS=\"" + phoneNumber + "\"\r");
  delay(1000);

  // Send the SMS content
  Serial2.print(smsMessage);
  Serial2.write(26); // Ctrl+Z character to send the message
  delay(1000);

  Serial.println("SMS sent to " + phoneNumber);
}
