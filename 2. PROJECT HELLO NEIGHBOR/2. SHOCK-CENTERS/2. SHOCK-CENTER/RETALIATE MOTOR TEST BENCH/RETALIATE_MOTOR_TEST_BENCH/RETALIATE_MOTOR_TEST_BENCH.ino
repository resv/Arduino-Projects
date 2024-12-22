#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// LCD configuration
#define LCD_WIDTH 170
#define LCD_HEIGHT 320
#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_CS 15
#define LCD_DC 2
#define LCD_RST 4
#define LCD_BLK 32

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

// Define the pins for the vibration motors
int motorPins[] = {13, 14, 12, 27, 26, 25, 33, 32, 35, 34, 23, 22, 21, 19, 18, 6, 4, 2, 15};
const int pinCount = sizeof(motorPins) / sizeof(motorPins[0]);

void setup() {
    Serial.begin(115200);

    // Initialize the LCD
    lcd.init(LCD_WIDTH, LCD_HEIGHT);
    lcd.setRotation(1);
    lcd.fillScreen(ST77XX_BLACK);
    lcd.setTextSize(2);
    lcd.setTextColor(ST77XX_WHITE);
    lcd.setCursor(0, 0);
    lcd.println("All Motors Vibrating");

    // Initialize all pins as OUTPUT and set them HIGH to activate motors
    for (int i = 0; i < pinCount; i++) {
        pinMode(motorPins[i], OUTPUT);
        digitalWrite(motorPins[i], HIGH); // Turn motor on
        Serial.println("Motor enabled on Pin: " + String(motorPins[i]));
    }
}

void loop() {
    // All motors are already running; no need for additional logic in the loop
}
