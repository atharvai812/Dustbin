#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""

#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "HX711.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>


#define IR_SENSOR_PIN 15
#define SERVO_PIN 13
#define BUZZER_PIN 27

Servo servoMotor;
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int trigPin = 5;
const int echoPin = 18;
const float DUSTBIN_DEPTH_CM = 30.0;
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

HX711 scale;
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;
const float WEIGHT_THRESHOLD = 10000.0; // Threshold weight in grams (10 kg)

char auth[] = "DwF17zDmM4eATunepCVHMEt1lXuJvKFI";
char ssid[] = "Atharva";
char pass[] = "ConformIT";

// Blynk virtual pin numbers
#define VPIN_LEVEL V1
#define VPIN_WEIGHT V2
#define VPIN_MESSAGE V0 // Virtual pin for displaying messages on Blynk

bool obstacleDetected = false;
unsigned long obstacleDetectedTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(IR_SENSOR_PIN, INPUT);
  servoMotor.attach(SERVO_PIN);
  servoMotor.write(0);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  lcd.init();
  lcd.backlight();

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(-107.25); // Calibrate the scale
  scale.tare(); // Reset the scale to 0

  Blynk.begin(auth, ssid, pass);

  // Display connected message on serial monitor
  Serial.println("Connected to Blynk server");

  // Display connected Wi-Fi network
  Serial.print("Connected to WiFi network: ");
  Serial.println(WiFi.SSID());

  // Display IP address
  Serial.print("Device IP address: ");
  Serial.println(WiFi.localIP());

  displayMessage("Use Me !");
}

void loop() {
  Blynk.run();

  long duration = measureDistance();
  float distanceCm = duration * SOUND_SPEED / 2;
  float levelPercent = max(0.0, 100.0 - ((distanceCm / DUSTBIN_DEPTH_CM) * 100.0));

  // Read weight from the load cell
  float weight = scale.get_units();
  if (weight < 0) weight = 0; // Ensure weight is not negative

  // Display dustbin level and weight on LCD
  displayLevelAndWeight(levelPercent, weight);

  // Check if the dustbin level exceeds 90%
  if (levelPercent > 90) {
    displayMessage("Please Empty Me");
    beep();
    Blynk.virtualWrite(VPIN_MESSAGE, "Please Empty Me"); // Send message to Blynk
  }

  // Check if weight exceeds threshold
  if (weight > WEIGHT_THRESHOLD) {
    displayMessage("Weight Exceeded!");
    beep(); // Activate the buzzer
    // Additional actions can be taken here if needed
    Blynk.virtualWrite(VPIN_MESSAGE, "Weight Exceeded!"); // Send message to Blynk
  }

  // Check if both level and weight are below their thresholds before opening the lid
  if (levelPercent <= 90 && weight <= WEIGHT_THRESHOLD && digitalRead(IR_SENSOR_PIN) == LOW) {
    if (!obstacleDetected) {
      rotateServo();
      obstacleDetected = true;
      obstacleDetectedTime = millis();
    }
    displayMessage("Feed me Garbage");
    beep();
    Blynk.virtualWrite(VPIN_MESSAGE, "Feed me Garbage"); // Send message to Blynk
  } else {
    if (obstacleDetected) {
      unsigned long currentTime = millis();
      if (currentTime - obstacleDetectedTime >= 1000) {
        servoMotor.write(0);
        displayMessage("Thank You!");
        delay(1000);
        obstacleDetected = false;
        Blynk.virtualWrite(VPIN_MESSAGE, "Thank You!"); // Send message to Blynk
      }
    }
  }
  
  delay(1000);
}

long measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  return pulseIn(echoPin, HIGH);
}

void displayLevelAndWeight(float levelPercent, float weightGrams) {
  // Convert weight from grams to kilograms
  float weightKg = weightGrams / 1000.0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Level: ");
  lcd.print(levelPercent);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("Weight: ");
  lcd.print(weightGrams); // Display weight in grams
  lcd.print(" g");

  // Update Blynk widgets
  Blynk.virtualWrite(VPIN_LEVEL, levelPercent);
  Blynk.virtualWrite(VPIN_WEIGHT, weightKg); // Send weight in kilograms to Blynk
}

void displayMessage(String message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
}

void rotateServo() {
  servoMotor.write(90);
}

void beep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
}
