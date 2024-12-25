#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  if (!mpu.begin()) {
    Serial.println("Failed to initialize MPU6050. Check wiring!");
    while (1);
  }

  // Set accelerometer range for better sensitivity
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G); // High sensitivity
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);   // Reduce noise
}

void loop() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  // Calculate vibration magnitude
  float vibration_magnitude = sqrt(
      pow(accel.acceleration.x, 2) +
      pow(accel.acceleration.y, 2) +
      pow(accel.acceleration.z, 2));

  // Subtract gravity (~9.8 m/s^2) for stationary cases
  float vibration_level = abs(vibration_magnitude - 9.8);

  Serial.print("Vibration Level: ");
  Serial.println(vibration_level);

  // Detect vibration threshold
  if (vibration_level > 1.10) { // Example threshold
    Serial.println("Vibration detected!");
  }

  delay(100); // Adjust sampling rate
}
