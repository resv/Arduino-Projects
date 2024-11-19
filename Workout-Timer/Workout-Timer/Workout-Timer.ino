#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define LCD_WIDTH 170
#define LCD_HEIGHT 320
#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_CS 15
#define LCD_DC 2
#define LCD_RST 4
#define LCD_BLK 32

const int buttonPin = 12;
const int ledPin = 13;
const int buzzerPin = 27;
const int timerInterval = 60;  // Timer interval in seconds
int timerRemaining = timerInterval;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
unsigned long lastTimerUpdate = 0;
unsigned long buttonHoldStartTime = 0;
bool timerRunning = false;
bool buttonHeld = false;
bool buttonPressShort = false;

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

bool buttonState = HIGH;
bool lastButtonState = HIGH;

// Threshold flags
bool threshold250msReached = false;
bool threshold500msReached = false;
bool threshold750msReached = false;

void setup() {
  lcd.init(LCD_WIDTH, LCD_HEIGHT);
  delay(500);
  lcd.setRotation(1);
  lcd.fillScreen(ST77XX_BLACK);

  lcd.setCursor(0, 0);
  lcd.setTextSize(3);
  lcd.setTextColor(ST77XX_WHITE);
  lcd.print("TIMER STANDING BY BUTTON 12        BUZZER 27        LED 13");

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  Serial.begin(115200);
  Serial.println("TIMER STANDING BY...");
}

void loop() {
  int reading = digitalRead(buttonPin);

  // Debounce logic
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {  // Button pressed
        buttonHoldStartTime = millis();
        buttonHeld = true;
        buttonPressShort = true;  // Button press is considered short

        digitalWrite(ledPin, HIGH);
        tone(buzzerPin, 1000);

      } else {  // Button released
        digitalWrite(ledPin, LOW);
        noTone(buzzerPin);
        buttonHeld = false;

        // Reset threshold flags
        threshold250msReached = false;
        threshold500msReached = false;
        threshold750msReached = false;

        if ((millis() - buttonHoldStartTime) < 750 && buttonPressShort) {
          // Handle short press
          if (timerRunning) {
            adjustTimerInterval();
          } else {
            startTimer();
          }
          buttonPressShort = false;  // Reset flag for next press
        }

        // Reset the timer if held for longer duration
        if ((millis() - buttonHoldStartTime) >= 750 && !threshold750msReached) {
          resetTimerToStandby();
        }
      }
    }
  }

  lastButtonState = reading;

  // Manage hold durations for buzzer tones
  handleHoldDurations();

  // Timer countdown logic
  if (timerRunning && (millis() - lastTimerUpdate >= 1000)) {
    countdownTimer();
  }
}

void adjustTimerInterval() {
  if (timerRemaining % 60 < 59) {
    timerRemaining = ((timerRemaining / 60) * 60) + 60;
  } else if (timerRemaining % 60 >= 59) {
    timerRemaining = ((timerRemaining / 60) * 60) + timerInterval;
  }

  int minutes = timerRemaining / 60;
  int seconds = timerRemaining % 60;

  lcd.fillRect(0, 80, LCD_HEIGHT, LCD_WIDTH, ST77XX_BLACK);
  lcd.setTextSize(3);
  lcd.setTextColor(ST77XX_CYAN);
  lcd.setCursor(15, 80);  // Position at the bottom row
  lcd.print("(EXTENDED ");
  if (minutes < 10) lcd.print("0");
  lcd.print(minutes);
  lcd.print(":");
  if (seconds < 10) lcd.print("0");
  lcd.print(seconds);
  lcd.print(")");

  Serial.print("-------- [ TIME EXTENDED ");
  if (minutes < 10) Serial.print("0");
  Serial.print(minutes);
  Serial.print(":");
  if (seconds < 10) Serial.print("0");
  Serial.print(seconds);
  Serial.println(" ] --------");
}

void startTimer() {
  lcd.fillRect(0, 0, LCD_HEIGHT, LCD_WIDTH, ST77XX_BLACK);
  lcd.setCursor(0, 0);
  Serial.println("----------- [ TIMER STARTED ] -----------");
 
  timerRunning = true;
  timerRemaining = timerInterval;
  lastTimerUpdate = millis();
}

void resetTimerToStandby() {
  noTone(buzzerPin);
  timerRunning = false;
  timerRemaining = timerInterval;

  lcd.fillRect(0, 0, LCD_HEIGHT, LCD_WIDTH, ST77XX_BLACK);
  lcd.setCursor(0, 0);
  lcd.setTextSize(6);
  lcd.setTextColor(ST77XX_BLUE);
  lcd.print("ADD SOMETING LATER");
  Serial.println("----------- [ TIMER SHUTDOWN ] -----------");
                  
  threshold750msReached = true;
}            

void handleHoldDurations() {
  if (buttonHeld) {
    unsigned long holdDuration = millis() - buttonHoldStartTime;

    if (holdDuration >= 750 && !threshold750msReached) {
      lcd.setTextSize(2);
      lcd.setCursor(0, 45);  // Position at the bottom row
      lcd.setTextColor(ST77XX_RED);
      lcd.print("[========================]");
      lcd.setTextSize(4);
      lcd.setCursor(5, 70);  // Position at the bottom row
      lcd.setTextColor(ST77XX_WHITE);
      lcd.print("SHUTTING DOWN");
      lcd.setTextSize(2);
      lcd.setCursor(0, 110);  // Position at the bottom row
      lcd.setTextColor(ST77XX_RED);
      lcd.print("[========================]");
      Serial.println("HOLDING: 0.5sec");

      resetTimerToStandby();
    } else if (holdDuration >= 500 && !threshold500msReached) {
      tone(buzzerPin, 100);

      lcd.setTextSize(2);
      lcd.setCursor(0, 45);  // Position at the bottom row
      lcd.setTextColor(ST77XX_RED);
      lcd.print("[========================]");
      lcd.setTextSize(4);
      lcd.setCursor(5, 70);  // Position at the bottom row
      lcd.setTextColor(ST77XX_WHITE);
      lcd.print("SHUTTING DOWN");
      lcd.setTextSize(2);
      lcd.setCursor(0, 110);  // Position at the bottom row
      lcd.setTextColor(ST77XX_RED);
      lcd.print("[========================]");
      Serial.println("HOLDING: 0.5sec");

      threshold500msReached = true;

    } else if (holdDuration >= 250 && !threshold250msReached) {
      tone(buzzerPin, 950);
      
      lcd.fillRect(0, 0, LCD_HEIGHT, LCD_WIDTH, ST77XX_BLACK);
      lcd.setTextSize(2);
      lcd.setCursor(0, 45);  // Position at the bottom row
      lcd.setTextColor(ST77XX_RED);
      lcd.print("[==========]");
      lcd.setTextSize(4);
      lcd.setCursor(5, 70);  // Position at the bottom row
      lcd.setTextColor(ST77XX_WHITE);
      lcd.print("SHUTTING DOWN");
      lcd.setTextSize(2);
      lcd.setCursor(168, 110);  // Position at the bottom row
      lcd.setTextColor(ST77XX_RED);
      lcd.print("[==========]");
      Serial.println("HOLDING: 0.25 sec");
      lcd.fillRect(0, 0, LCD_HEIGHT, LCD_WIDTH, ST77XX_BLACK);
      
      threshold250msReached = true;
    }
  }
}

void countdownTimer() {
  lastTimerUpdate = millis();
  timerRemaining--;

  int minutes = timerRemaining / 60;
  int seconds = timerRemaining % 60;

  lcd.fillRect(0, 0, 320, 80, ST77XX_BLACK);
  lcd.setCursor(16, 0);
  lcd.setTextColor(ST77XX_GREEN);
  lcd.setTextSize(10);
  if (minutes < 10) lcd.print("0");
  lcd.print(minutes);
  lcd.setTextColor(ST77XX_WHITE);
  lcd.print(":");
  lcd.setTextColor(ST77XX_GREEN);
  if (seconds < 10) lcd.print("0");
  lcd.print(seconds);

  Serial.print("                  ");
  if (minutes < 10) Serial.print("0");
  Serial.print(minutes);
  Serial.print(":");
  if (seconds < 10) Serial.print("0");
  Serial.println(seconds);

  if (timerRemaining <= 0) {
    endTimerSequence();
  }
}

void endTimerSequence() {
  timerRunning = false;
  lcd.fillRect(0, 0, LCD_HEIGHT, LCD_WIDTH, ST77XX_BLACK);
  lcd.setTextSize(2);
  lcd.setCursor(0, 45);
  lcd.setTextColor(ST77XX_RED);
  lcd.print("[========================]");

  lcd.setCursor(17, 65);
  lcd.setTextColor(ST77XX_WHITE);
  lcd.setTextSize(5);
  lcd.print("TIME'S UP!");

  lcd.setTextSize(2);
  lcd.setCursor(0, 110);
  lcd.setTextColor(ST77XX_RED);
  lcd.print("[========================]");

  Serial.println("TIMER FINISHED!");

  for (int cycle = 0; cycle < 3; cycle++) {
    for (int i = 0; i < 5; i++) {
      digitalWrite(ledPin, HIGH);
      tone(buzzerPin, 1000);
      delay(50);
      digitalWrite(ledPin, LOW);
      noTone(buzzerPin);
      delay(50);
    }
    delay(500);
  }
}