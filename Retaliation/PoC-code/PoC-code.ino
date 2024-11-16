#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// LCD pin definitions
#define LCD_WIDTH  170
#define LCD_HEIGHT 320
#define LCD_MOSI 23  
#define LCD_SCLK 18  
#define LCD_CS   15  
#define LCD_DC    2  
#define LCD_RST   4  
#define LCD_BLK   32  

// Button, LED, and Buzzer pin definitions
const int buttonPin = 12;  // Button pin
const int ledPin = 13;     // LED pin
const int buzzerPin = 27;  // Buzzer pin

// LCD object
Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

// Variables to track button state
bool buttonState = HIGH;
bool lastButtonState = HIGH;

void setup() {
  // Initialize LCD
  lcd.init(LCD_WIDTH, LCD_HEIGHT);
  delay(500);
  lcd.setRotation(1);  
  lcd.fillScreen(ST77XX_BLACK);

  // Display initial text
  lcd.setCursor(10, 10);
  lcd.setTextSize(2);
  lcd.setTextColor(ST77XX_WHITE);
  lcd.print("Hello World!");

  // Setup button, LED, and buzzer
  pinMode(buttonPin, INPUT_PULLUP); // Use INPUT_PULLUP to handle button
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // Display initial button status
  lcd.setCursor(10, 60);
  lcd.print("Button off");

  //Initialize serial monitor
  Serial.begin(115200);
  Serial.print("Workout Timer Standing By...");
}

void loop() {
  // Read button state
  buttonState = digitalRead(buttonPin);
  
  // Only update if button state has changed
  if (buttonState != lastButtonState) {
    if (buttonState == LOW) { // LOW because INPUT_PULLUP reads LOW when pressed
      digitalWrite(ledPin, HIGH);   // Turn on LED
      tone(buzzerPin, 1000);        // Sound the buzzer at 1000 Hz

      // Display "Button on" when pressed
      lcd.fillRect(10, 60, LCD_WIDTH - 20, 30, ST77XX_BLACK); // Clear previous text
      lcd.setCursor(10, 60);
      lcd.print("Button on");
    } else {
      digitalWrite(ledPin, LOW);    // Turn off LED
      noTone(buzzerPin);            // Turn off buzzer

      // Display "Button off" when not pressed
      lcd.fillRect(10, 60, LCD_WIDTH - 20, 30, ST77XX_BLACK); // Clear previous text
      lcd.setCursor(10, 60);
      lcd.print("Button off");
    }

    // Update last button state
    lastButtonState = buttonState;
  }
}
