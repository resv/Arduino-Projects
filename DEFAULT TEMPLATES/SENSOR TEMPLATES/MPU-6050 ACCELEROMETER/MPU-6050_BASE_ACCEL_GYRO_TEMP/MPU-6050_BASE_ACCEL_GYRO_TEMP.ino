#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

#define MPU_POWER_PIN 0 // GPIO0 to supply 3.3V power to MPU6050

// Global ESP variables
const char* thisClientID = "RESV-SHOCKERA"; // Define the ClientID
String isArmed = "DISARMED";
String dateDate = "MM/DD";
String dateTime = "HH:MM:SS";

// Global Sensor variables
float baselineX = 0, baselineY = 0, baselineZ = 0; // Baseline values
float shockThreshold = 0.5; // Threshold for shock detection
int temperatureC = 0; // Temperature in Celsius (whole number)
int temperatureF = 0; // Temperature in Fahrenheit (whole number)
unsigned long lastShockTime = 0; // Time of the last detected shock
const unsigned long recalibrationInterval = 600000; // 10 minutes in milliseconds
const unsigned long readAndSerialInterval = 100; // 100ms interval for sensor read and Serial output

// Variables for non-blocking recalibration
bool shockDetected = false;
bool recalibrating = false;
unsigned long recalibrationStart = 0;
unsigned long lastReadAndSerialTime = 0; // Tracks the last time sensor was read and Serial output

int recalibrationSampleCount = 0;
float sumX = 0, sumY = 0, sumZ = 0;

// Function to start recalibration
void startRecalibration() {
  recalibrating = true;
  recalibrationStart = millis();
  recalibrationSampleCount = 0;
  sumX = 0;
  sumY = 0;
  sumZ = 0;
  Serial.println("Recalibrating baseline...");
}

// Function to handle recalibration logic
void handleRecalibration() {
  if (recalibrating) {
    unsigned long currentTime = millis();
    if (currentTime - lastReadAndSerialTime >= readAndSerialInterval) { // Use the same interval
      lastReadAndSerialTime = currentTime;

      sensors_event_t accel, gyro, temp;
      mpu.getEvent(&accel, &gyro, &temp);

      sumX += accel.acceleration.x;
      sumY += accel.acceleration.y;
      sumZ += accel.acceleration.z;
      recalibrationSampleCount++;

      if (recalibrationSampleCount >= 40) { // Collect 40 samples over ~4 seconds (100ms interval)
        baselineX = sumX / recalibrationSampleCount;
        baselineY = sumY / recalibrationSampleCount;
        baselineZ = sumZ / recalibrationSampleCount;

        Serial.println("Baseline recalibrated:");
        Serial.print("Baseline X: "); Serial.println(baselineX, 2);
        Serial.print("Baseline Y: "); Serial.println(baselineY, 2);
        Serial.print("Baseline Z: "); Serial.println(baselineZ, 2);

        recalibrating = false; // End recalibration
      }
    }
  }
}

// Setup function
void setup() {
  Serial.begin(115200);

  // Configure GPIO0 as output to power the MPU6050
  pinMode(MPU_POWER_PIN, OUTPUT);
  digitalWrite(MPU_POWER_PIN, HIGH); // Supply power to MPU6050
  delay(100); // Allow MPU6050 to stabilize after power-up

  // Initialize I2C with swapped SDA and SCL pins
  Wire.begin(9, 8); // SDA = GPIO9, SCL = GPIO8

  Serial.println("Initializing MPU6050...");
  if (!mpu.begin()) {
    Serial.println("Failed to initialize MPU6050. Check wiring!");
    while (1);
  }

  // Set accelerometer range for better sensitivity
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G); // High sensitivity
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);   // Reduce noise

  // Initial baseline calculation
  startRecalibration();
  lastShockTime = millis(); // Record the time after the initial calibration
}

// Function to print sensor data
void printSensorData(bool shockDetected, float vibrationMagnitude, sensors_event_t& accel, sensors_event_t& gyro) {
  Serial.print("|CID:"); Serial.print(thisClientID);

  Serial.print("|DD:");
  Serial.print(dateDate);

  Serial.print("|DT:");
  Serial.print(dateTime);

  Serial.print("|S:");
  Serial.print(shockDetected ? "DETECTED!" : "LISTENING");

  Serial.print("|VM:");
  Serial.print(vibrationMagnitude, 2);

  Serial.print("|IA:");
  Serial.print(isArmed);

  Serial.print("|AX:");
  Serial.print(accel.acceleration.x, 2);
  Serial.print("|AY:");
  Serial.print(accel.acceleration.y, 2);
  Serial.print("|AZ:");
  Serial.print(accel.acceleration.z, 2);

  Serial.print("|GX:");
  Serial.print(gyro.gyro.x, 2);
  Serial.print("|GY:");
  Serial.print(gyro.gyro.y, 2);
  Serial.print("|GZ:");
  Serial.print(gyro.gyro.z, 2);

  Serial.print("|TC:");
  Serial.print(temperatureC);
  Serial.print("|TF:");
  Serial.println(temperatureF);
}

// Main loop function
void loop() {
  // Handle recalibration logic
  handleRecalibration();

  // Skip shock detection during recalibration
  if (recalibrating) {
    return;
  }

  unsigned long currentTime = millis();
  if (currentTime - lastReadAndSerialTime >= readAndSerialInterval) {
    lastReadAndSerialTime = currentTime;

    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    // Update temperature variables if the whole number changes
    int newTempC = static_cast<int>(temp.temperature);
    int newTempF = static_cast<int>((temp.temperature * 9.0 / 5.0) + 32.0);

    if (newTempC != temperatureC || newTempF != temperatureF) {
      temperatureC = newTempC;
      temperatureF = newTempF;
    }

    // Calculate vibration deviation from baseline
    float deviationX = accel.acceleration.x - baselineX;
    float deviationY = accel.acceleration.y - baselineY;
    float deviationZ = accel.acceleration.z - baselineZ;

    // Calculate vibration magnitude
    float vibrationMagnitude = sqrt(pow(deviationX, 2) + pow(deviationY, 2) + pow(deviationZ, 2));

    // Check for shock detection
    shockDetected = vibrationMagnitude > shockThreshold;

    if (shockDetected) {
      startRecalibration(); // Start recalibration after detecting a shock
      lastShockTime = millis(); // Reset the last shock time
    }

    // Recalibrate baseline if no shock is detected for 10 minutes
    if (millis() - lastShockTime >= recalibrationInterval) {
      Serial.println("Recalibration triggered due to 10-minute threshold.");
      startRecalibration();
      lastShockTime = millis(); // Reset the last shock time after recalibration
    }

    // Print sensor data
    printSensorData(shockDetected, vibrationMagnitude, accel, gyro);
  }
}
