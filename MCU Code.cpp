#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include "ThingSpeak.h"

#define DISPLAY_WIDTH 128  // OLED display width in pixels
#define DISPLAY_HEIGHT 64  // OLED display height in pixels
#define OLED_RESET_PIN -1   // Reset pin number (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, OLED_RESET_PIN);

const char* wifiSSID = "HeartBeatMonitor";  // Network SSID
const char* wifiPassword = "SecurePassword123";  // Network password

int pulseRate;

WiFiClient client;

unsigned long channelID = 1234567;  // Replace with your ThingSpeak channel ID
const char* writeAPIKey = "YOUR_WRITE_API_KEY";  // Replace with your ThingSpeak write API key

#include "esp_adc_cal.h"

unsigned long lastMeasurementTime = 0;
unsigned long measurementInterval = 30000;  // 30 seconds

#define TEMPERATURE_SENSOR_PIN 39

int xAxisPin = 34;
int yAxisPin = 35;

int rawTemperatureReading = 0;
float temperatureCelsius = 0.0;
float temperatureFahrenheit = 0.0;
float voltage = 0.0;

int fallDetectionPin = 36;

int fallCount = 0;
unsigned long fallDetectionStartTime = 0;

enum FallDirection { NO_FALL, FORWARD_FALL, BACKWARD_FALL, LEFT_FALL, RIGHT_FALL };
FallDirection detectedFall = NO_FALL;

void setup() {
  Serial.begin(9600);

  WiFi.mode(WIFI_STA);

  ThingSpeak.begin(client);  // Initialize ThingSpeak communication

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(wifiSSID, wifiPassword);
      delay(5000);
    }
    Serial.println("Connected!");
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Failed to initialize OLED display");
    while (1); // Halt program execution
  }

  display.display();
  delay(100);
  display.clearDisplay();
}

void loop() {
  rawTemperatureReading = analogRead(TEMPERATURE_SENSOR_PIN);

  // Calibrate ADC and get voltage in millivolts
  voltage = readADC_Cal(rawTemperatureReading);

  // Temperature in Celsius = Voltage (mV) / 10
  temperatureCelsius = voltage / 10;
  temperatureFahrenheit = (temperatureCelsius * 1.8) + 32;

  int xAxisReading = analogRead(xAxisPin);
  delay(1);

  int yAxisReading = analogRead(yAxisPin);
  delay(1);

  fallDetectionStartTime = millis();

  while (millis() < (fallDetectionStartTime + 10000)) {
    if (analogRead(fallDetectionPin) < 100) {
      fallCount++;
      while (analogRead(fallDetectionPin) < 100);
    }
  }

  fallCount = fallCount * 6;

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(0, 30);
  display.print("Pulse:");
  display.print(fallCount);

  display.setCursor(0, 10);
  display.print("Temp:");
  display.print(temperatureCelsius);
  display.print("°C");

  detectedFall = NO_FALL;
  if (xAxisReading > 1500 && xAxisReading < 1800) {
    display.setTextSize(1);
    display.setCursor(0, 50);
    display.print("Fall:");
    display.print("Forward");
    detectedFall = FORWARD_FALL;
