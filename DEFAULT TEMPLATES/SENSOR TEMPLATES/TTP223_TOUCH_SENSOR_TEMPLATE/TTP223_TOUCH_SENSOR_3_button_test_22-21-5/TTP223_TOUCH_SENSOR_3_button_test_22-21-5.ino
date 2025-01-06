#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// LCD Configuration
#define LCD_WIDTH 170
#define LCD_HEIGHT 320
#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_CS 15
#define LCD_DC 2
#define LCD_RST 4
#define LCD_BLK 32

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

// Touch Sensor Pins (Updated)
#define TOUCH_SENSOR_PIN_1 25 // Sensor 1 (GPIO 25)
#define TOUCH_SENSOR_PIN_2 22 // Sensor 2 (GPIO 22)
#define TOUCH_SENSOR_PIN_3 21 // Sensor 3 (GPIO 21)
#define TOUCH_SENSOR_PIN_4 26 // Sensor 4 (GPIO 26)
#define TOUCH_SENSOR_PIN_5 19 // Sensor 5 (GPIO 19)
#define TOUCH_SENSOR_PIN_6 5  // Sensor 6 (GPIO 5)

// Long Press Threshold (in milliseconds)
#define LONG_PRESS_THRESHOLD 1500

// Variables for touch sensor states
bool touchActive[6] = {false, false, false, false, false, false}; // Active state for each sensor
unsigned long touchStartTime[6] = {0, 0, 0, 0, 0, 0};             // Start time for each sensor
unsigned long touchDuration[6] = {0, 0, 0, 0, 0, 0};              // Duration for each sensor

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);

    // Initialize the LCD
    lcd.init(LCD_WIDTH, LCD_HEIGHT);
    lcd.setRotation(3);
    lcd.fillScreen(ST77XX_BLACK);
    lcd.setTextColor(ST77XX_WHITE);
    lcd.setTextSize(2);

    // Initialize the touch sensor pins
    pinMode(TOUCH_SENSOR_PIN_1, INPUT);
    pinMode(TOUCH_SENSOR_PIN_2, INPUT);
    pinMode(TOUCH_SENSOR_PIN_3, INPUT);
    pinMode(TOUCH_SENSOR_PIN_4, INPUT);
    pinMode(TOUCH_SENSOR_PIN_5, INPUT);
    pinMode(TOUCH_SENSOR_PIN_6, INPUT);

    // Display startup message
    lcd.setCursor(10, 10);
    lcd.println("Touch Test");
    lcd.setCursor(10, 40);
    lcd.println("Pins: 25, 22, 21, 26, 19, 5");
}

void loop() {
    // Read touch states for each sensor
    handleTouchSensor(0, TOUCH_SENSOR_PIN_1);
    handleTouchSensor(1, TOUCH_SENSOR_PIN_2);
    handleTouchSensor(2, TOUCH_SENSOR_PIN_3);
    handleTouchSensor(3, TOUCH_SENSOR_PIN_4);
    handleTouchSensor(4, TOUCH_SENSOR_PIN_5);
    handleTouchSensor(5, TOUCH_SENSOR_PIN_6);

    // Small delay to avoid excessive updates
    delay(50);
}

// Function to handle touch for a specific sensor
void handleTouchSensor(int sensorIndex, int sensorPin) {
    bool currentTouchState = digitalRead(sensorPin) == HIGH; // Active HIGH

    // Handle touch press detection
    if (currentTouchState && !touchActive[sensorIndex]) {
        // Touch started
        touchActive[sensorIndex] = true;
        touchStartTime[sensorIndex] = millis();
        Serial.print("Touch Started on Sensor ");
        Serial.print(sensorIndex + 1);
        Serial.print(" (Pin ");
        Serial.print(sensorPin);
        Serial.println(")");
    }

    if (!currentTouchState && touchActive[sensorIndex]) {
        // Touch ended
        touchActive[sensorIndex] = false;
        touchDuration[sensorIndex] = millis() - touchStartTime[sensorIndex];

        // Determine press type
        String pressType = (touchDuration[sensorIndex] >= LONG_PRESS_THRESHOLD) ? "Long Press" : "Short Press";

        // Debug to Serial Monitor
        Serial.print("Touch Duration on Sensor ");
        Serial.print(sensorIndex + 1);
        Serial.print(" (Pin ");
        Serial.print(sensorPin);
        Serial.print("): ");
        Serial.print(touchDuration[sensorIndex]);
        Serial.print(" ms - ");
        Serial.println(pressType);

        // Display activated pin and press type on the LCD
        lcd.fillRect(10, 80, 300, 60, ST77XX_BLACK); // Clear the display area
        lcd.setCursor(10, 80);
        lcd.setTextColor(ST77XX_WHITE);
        lcd.print("Pin: ");
        lcd.println(sensorPin);

        lcd.setCursor(10, 110);
        lcd.setTextColor((pressType == "Long Press") ? ST77XX_GREEN : ST77XX_RED);
        lcd.println(pressType);
    }

    // Update live touch state on LCD
    int liveYOffset = 200 + (sensorIndex * 20); // Offset each sensor's live status
    lcd.fillRect(10, liveYOffset, 300, 20, ST77XX_BLACK); // Clear the live status area
    lcd.setCursor(10, liveYOffset);
    if (currentTouchState) {
        lcd.setTextColor(ST77XX_BLUE);
        lcd.print("Pin ");
        lcd.print(sensorPin);
        lcd.println(": Active");
    } else {
        lcd.setTextColor(ST77XX_RED);
        lcd.print("Pin ");
        lcd.print(sensorPin);
        lcd.println(": No Touch");
    }
}
