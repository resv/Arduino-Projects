// Pin connected to the TTP223 SIG pin
#define TOUCH_PIN 2

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Set TOUCH_PIN as input
  pinMode(TOUCH_PIN, INPUT);
  Serial.println("TTP223 Touch Sensor is running, serial will only fire if touched!");
}

void loop() {
  // Read the touch sensor state
  int touchState = digitalRead(TOUCH_PIN);

  // Print state to the Serial Monitor
  if (touchState == HIGH) {
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!Touched!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
  } else {
    Serial.println("sensing");
  }

  delay(100); // Short delay to avoid spamming the output
}
