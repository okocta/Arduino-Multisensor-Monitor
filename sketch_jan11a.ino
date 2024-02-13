#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

const int buttonPin = 8;        // Digital pin connected to the push button
const int pulseSensorPin = A0;  // Analog pin connected to the pulse sensor


int buttonState = LOW;
int lastButtonState = LOW;
bool waitForTempHumidity = false;
bool waitForGas = false;
bool waitForBPM = false;  // Flag to control the display of the BMP screen
const int AO_Pin = A1; // Connect the AO of MQ-4 sensor with analog channel 2 pin (A2) of Arduino
const int DO_Pin = 9;  // Connect the DO of MQ-4 sensor with digital pin 9 (D9) of Arduino
const int LED_Pin = 13; // Connect an LED with D13 pin of Arduino
int threshold = 200; 
int A0_Out; // Create a variable to store the analog output of the MQ-4 sensor

#define DHT_PIN 7    // Digital pin connected to the DHT sensor
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

enum DisplayState {
  WAITING,
  TEMP_HUMIDITY,
  BPM,
  GAS
};

DisplayState currentState = WAITING;

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 3000;  // Update every 3 seconds
const unsigned long gasDisplayDuration = 10000;  // Display Gas Concentration for 10 seconds
const unsigned long serialPrintInterval = 1000;  // Print to Serial Monitor every 1 second
unsigned long lastSerialPrintTime = 0;

void setup() {
  Serial.begin(9600);
  pinMode(DO_Pin, INPUT); // Set the D9 pin as a digital input pin
  pinMode(LED_Pin, OUTPUT); // Set the D13 pin as a digital output pin

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Waiting for command");
  display.display();

  pinMode(buttonPin, INPUT);
  dht.begin();
}

void loop() {
  unsigned long currentTime = millis();

  buttonState = digitalRead(buttonPin);

  if (buttonState == HIGH && lastButtonState == LOW) {
    // Button pressed: Toggle between temperature/humidity, BPM, and Gas
    switch (currentState) {
      case WAITING:
        currentState = TEMP_HUMIDITY;
        break;
      case TEMP_HUMIDITY:
        currentState = BPM;
        break;
      case BPM:
        currentState = GAS;
        break;
      case GAS:
        currentState = TEMP_HUMIDITY;
        break;
    }

    waitForTempHumidity = false;  
    waitForBPM = false;
    waitForGas = false;

    while (digitalRead(buttonPin) == HIGH) {
      delay(10);
    }
  }

  // Update data based on the current state
  switch (currentState) {
    case WAITING:
      // Display waiting message
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Waiting for command");
      display.display();
      break;

    case TEMP_HUMIDITY:
      displayTempHumidity();
      break;

    case BPM:
      displayBPM();
      break;

    case GAS:
      readGasConcentration();  // Read gas concentration only when in GAS state
      displayGasConcentration();
      break;
  }

  // Print gas concentration to Serial Monitor every 1 second
  if (currentTime - lastSerialPrintTime >= serialPrintInterval && currentState == GAS) {
    Serial.print("Gas Concentration: ");
    Serial.println(A0_Out);
    lastSerialPrintTime = currentTime;
  }

  lastButtonState = buttonState;
}

void displayTempHumidity() {
  display.clearDisplay();
  display.setCursor(0, 0);
  
  float actualTemperature = dht.readTemperature();
  float adjustedTemperature = actualTemperature - 3.0;  // Adjust temperature by subtracting 3 degrees
  
  display.print("Temperature: ");
  display.print(adjustedTemperature);
  display.print(" C");

  display.setCursor(0, 10);
  display.print("Humidity: ");
  display.print(dht.readHumidity());
  display.print(" %");

  display.display();
}


void displayBPM() {
  int pulseValue = analogRead(pulseSensorPin);
  int bpm = map(pulseValue, 0, 1023, 40, 110);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Waiting for BPM sensor");

  display.setCursor(0, 20);
  display.print("BPM: ");
  display.print(bpm);

  display.display();
}

void readGasConcentration() {
  A0_Out = analogRead(AO_Pin); // Read the analog output measurement sample from the MQ4 sensor's AO pin
}

void displayGasConcentration() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Threshold: ");
  display.print(threshold);
  display.print(", ");

  display.print("Methane Concentration: ");
  display.println(A0_Out);

  display.display();

  // Turn on/off LED based on gas concentration and threshold
  if (A0_Out > threshold) {
    digitalWrite(LED_Pin, HIGH);  // If the concentration is above the threshold, turn on the LED
  } else {
    digitalWrite(LED_Pin, LOW);  // If the concentration is below or equal to the threshold, turn off the LED
  }
}