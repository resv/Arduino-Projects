const int LED_PIN = 8;
const int BUZZER_PIN = 9;
const int ARCADE_BUTTON = 10;      // The pin that the button is connected to
const int VIBRATION_MOTOR_PIN = 11; // The pin that the MOSFET gate for the motor is connected to

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ARCADE_BUTTON, INPUT_PULLUP); // Set the button pin as input with internal pull-up resistor
  pinMode(VIBRATION_MOTOR_PIN, OUTPUT); // Set the motor pin as output
}

void loop() {
  bool buttonState = digitalRead(ARCADE_BUTTON); // Read the button state

  // If the button is pressed (LOW because we use INPUT_PULLUP)
  if (buttonState == LOW) {
    digitalWrite(LED_PIN, HIGH);        // Turn the LED on
    tone(BUZZER_PIN, 1000);             // Play a 1000 Hz tone
    digitalWrite(VIBRATION_MOTOR_PIN, HIGH); // Activate the vibration motor
  } else {
    digitalWrite(LED_PIN, LOW);         // Turn the LED off
    noTone(BUZZER_PIN);                 // Stop the tone
    digitalWrite(VIBRATION_MOTOR_PIN, LOW);  // Deactivate the vibration motor
  }
}
