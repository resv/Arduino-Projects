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
#define LCD_BLK 32 // Backlight pin

#define ONBOARD_BUTTON_PIN 0 // Onboard button for toggling

Adafruit_ST7789 tft = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

bool backlightState = true; // Track backlight state
int lastButtonState = HIGH; // Previous state of the button

void setup() {
  pinMode(LCD_BLK, OUTPUT);      // Configure backlight pin as output
  pinMode(ONBOARD_BUTTON_PIN, INPUT_PULLUP); // Configure button with pull-up

  digitalWrite(LCD_BLK, HIGH);  // Turn on the backlight initially
  tft.init(LCD_WIDTH, LCD_HEIGHT);
  tft.setRotation(1);           // Adjust as needed
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 20);
  tft.print("Backlight Toggle");
}

void loop() {
  int currentButtonState = digitalRead(ONBOARD_BUTTON_PIN);

  // Check for state change (HIGH -> LOW)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    backlightState = !backlightState; // Toggle backlight state
    digitalWrite(LCD_BLK, backlightState ? HIGH : LOW); // Control backlight
  }

  lastButtonState = currentButtonState; // Update the last button state
}
