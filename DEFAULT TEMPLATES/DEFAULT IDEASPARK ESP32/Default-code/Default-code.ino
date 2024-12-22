#include <Adafruit_GFX.h>    // Importing the Adafruit_GFX library
#include <Adafruit_ST7789.h> // Import the Adafruit_ST7789 library

// Define the size of the screen
#define LCD_WIDTH  170
#define LCD_HEIGHT 320

// Define the pins of the ESP32 connected to the LCD
#define LCD_MOSI 23  // SDA Pin on ESP32 D23
#define LCD_SCLK 18  // SCL Pin on ESP32 D18
#define LCD_CS   15  // Chip select control pin on ESP32 D15
#define LCD_DC    2  // Data Command control pin on ESP32 D2
#define LCD_RST   4  // Reset pin (could connect to RST pin) on ESP32 D4
#define LCD_BLK   32  // Black Light Pin on ESP32 D32

// Vibration sensor and motor pins
const int sensorPin = 13;    // Pin connected to vibration sensor output
const int motorPin = 14;     // Pin connected to motor control

// Create the Adafruit_ST7789 object
Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

// Variable to keep track of the display state
bool detectionDisplayed = false;

void setup() {
  // Initialize LCD
  lcd.init(LCD_WIDTH, LCD_HEIGHT);
  lcd.setRotation(0);  
  lcd.fillScreen(ST77XX_BLACK);

  // Set up text properties
  lcd.setTextSize(2);
  lcd.setTextColor(ST77XX_WHITE);
  
  // Initialize sensor and motor pins
  pinMode(sensorPin, INPUT);
  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);  // Ensure motor is off initially

  // Display initial "Listening" message
  lcd.setCursor(10, 10);
  lcd.print("Listening");
}

void loop() {
  // Read the vibration sensor
  int sensorValue = digitalRead(sensorPin);

  // If vibration detected
  if (sensorValue == HIGH) {
    if (!detectionDisplayed) { // Only update if not already displaying "Detection"
      lcd.fillScreen(ST77XX_BLACK);    // Clear the screen
      lcd.setCursor(10, 10);           // Position for the "Detection" message
      lcd.print("Detection");          // Display "Detection"
      detectionDisplayed = true;       // Update flag
    }

    // Activate motor for 1 second
    digitalWrite(motorPin, HIGH);      // Turn motor on
    delay(1000);                       // Run motor for 1 second
    digitalWrite(motorPin, LOW);       // Turn motor off
  } 
  else { // No vibration detected
    if (detectionDisplayed) { // Only update if currently displaying "Detection"
      lcd.fillScreen(ST77XX_BLACK);    // Clear the screen
      lcd.setCursor(10, 10);           // Position for the "Listening" message
      lcd.print("Listening");          // Display "Listening"
      detectionDisplayed = false;      // Update flag
    }
  }

  delay(200); // Small delay to stabilize sensor reading
}
