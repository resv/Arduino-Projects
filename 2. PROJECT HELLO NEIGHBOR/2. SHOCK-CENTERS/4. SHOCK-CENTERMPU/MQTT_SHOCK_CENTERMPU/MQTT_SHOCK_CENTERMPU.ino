#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFi.h> // Add Wi-Fi library
#include "esp_log.h"
#include <WiFiClientSecure.h> // For secure SSL connection
#include <PubSubClient.h>   // For MQTT client

Adafruit_MPU6050 mpu;
#define MPU_POWER_PIN 0 // GPIO0 to supply 3.3V power to MPU6050

// Global ESP variables
const char* thisClientID = "RESV-SHOCKERA"; // Define the ClientID
String isArmed = "DISARMED";
String dateDate = "MM/DD";
String dateTime = "HH:MM:SS";
String event = "-";

// Global Sensor variables
float baselineX = 0, baselineY = 0, baselineZ = 0; // Baseline values
float vibrationThreshold = 0.5; // Threshold for shock detection
int temperatureC = 0; // Temperature in Celsius (whole number)
int temperatureF = 0; // Temperature in Fahrenheit (whole number)
unsigned long lastShockTime = 0; // Time of the last detected shock
const unsigned long recalibrationInterval = 600000; // 10 minutes in milliseconds
const unsigned long readAndSerialInterval = 100; // 100ms interval for sensor read and Serial output

// NTP Variables
unsigned long lastNTPFetchMillis = 0; // Tracks the last NTP fetch time
unsigned long internalClockMillis = 0; // Tracks time since last internal clock update
const unsigned long internalClockInterval = 1000; // 1-second interval for updating the internal clock
time_t internalEpochTime = 0; // Internal clock epoch time in seconds

// Variables for non-blocking recalibration
bool shockDetected = false;
bool recalibrating = false;
unsigned long recalibrationStart = 0;
unsigned long lastReadAndSerialTime = 0; // Tracks the last time sensor was read and Serial output

int recalibrationSampleCount = 0;
float sumX = 0, sumY = 0, sumZ = 0;

// Wi-Fi VARIABLES
const char* wifi_credentials[][2] = {
    {"OP9", "aaaaaaaaa1"},
    {"icup +1", "aaaaaaaaa1"},
    {"ICU", "Emmajin6!"}
};
const int num_wifi_credentials = sizeof(wifi_credentials) / sizeof(wifi_credentials[0]);
int currentWiFiIndex = 0; // Index of the current SSID to try
static unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 5000; // Check Wi-Fi every 5 seconds

// MQTT broker credentials
const char* mqtt_server = "9321cdfa0af34b83b77797a4488354cd.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "MasterA";
const char* mqtt_password = "MasterA1";
const char* mqtt_topic_CENTRAL_HUB = "CENTRAL-HUB";
const char* mqtt_topic_SHOCK_CENTER = "SHOCK-CENTER";
const char* clientID = "RESV-SHOCKERA";
WiFiClientSecure espClient; // MQTT OVER SSL
PubSubClient client(espClient); // creates instance object client for PubSubClient class
// Non-blocking reconnect variables
unsigned long mqttReconnectTimer = 0;
const unsigned long mqttReconnectInterval = 5000; // Retry every 5 seconds


// Root certificate
const char* root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFBjCCAu6gAwIBAgIRAIp9PhPWLzDvI4a9KQdrNPgwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAw
WhcNMjcwMzEyMjM1OTU5WjAzMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg
RW5jcnlwdDEMMAoGA1UEAxMDUjExMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB
CgKCAQEAuoe8XBsAOcvKCs3UZxD5ATylTqVhyybKUvsVAbe5KPUoHu0nsyQYOWcJ
DAjs4DqwO3cOvfPlOVRBDE6uQdaZdN5R2+97/1i9qLcT9t4x1fJyyXJqC4N0lZxG
AGQUmfOx2SLZzaiSqhwmej/+71gFewiVgdtxD4774zEJuwm+UE1fj5F2PVqdnoPy
6cRms+EGZkNIGIBloDcYmpuEMpexsr3E+BUAnSeI++JjF5ZsmydnS8TbKF5pwnnw
SVzgJFDhxLyhBax7QG0AtMJBP6dYuC/FXJuluwme8f7rsIU5/agK70XEeOtlKsLP
Xzze41xNG/cLJyuqC0J3U095ah2H2QIDAQABo4H4MIH1MA4GA1UdDwEB/wQEAwIB
hjAdBgNVHSUEFjAUBggrBgEFBQcDAgYIKwYBBQUHAwEwEgYDVR0TAQH/BAgwBgEB
/wIBADAdBgNVHQ4EFgQUxc9GpOr0w8B6bJXELbBeki8m47kwHwYDVR0jBBgwFoAU
ebRZ5nu25eQBc4AIiMgaWPbpm24wMgYIKwYBBQUHAQEEJjAkMCIGCCsGAQUFBzAC
hhZodHRwOi8veDEuaS5sZW5jci5vcmcvMBMGA1UdIAQMMAowCAYGZ4EMAQIBMCcG
A1UdHwQgMB4wHKAaoBiGFmh0dHA6Ly94MS5jLmxlbmNyLm9yZy8wDQYJKoZIhvcN
AQELBQADggIBAE7iiV0KAxyQOND1H/lxXPjDj7I3iHpvsCUf7b632IYGjukJhM1y
v4Hz/MrPU0jtvfZpQtSlET41yBOykh0FX+ou1Nj4ScOt9ZmWnO8m2OG0JAtIIE38
01S0qcYhyOE2G/93ZCkXufBL713qzXnQv5C/viOykNpKqUgxdKlEC+Hi9i2DcaR1
e9KUwQUZRhy5j/PEdEglKg3l9dtD4tuTm7kZtB8v32oOjzHTYw+7KdzdZiw/sBtn
UfhBPORNuay4pJxmY/WrhSMdzFO2q3Gu3MUBcdo27goYKjL9CTF8j/Zz55yctUoV
aneCWs/ajUX+HypkBTA+c8LGDLnWO2NKq0YD/pnARkAnYGPfUDoHR9gVSp/qRx+Z
WghiDLZsMwhN1zjtSC0uBWiugF3vTNzYIEFfaPG7Ws3jDrAMMYebQ95JQ+HIBD/R
PBuHRTBpqKlyDnkSHDHYPiNX3adPoPAcgdF3H2/W0rmoswMWgTlLn1Wu0mrks7/q
pdWfS6PJ1jty80r2VKsM/Dj3YIDfbjXKdaFU5C+8bhfJGqU3taKauuz0wHVGT3eo
6FlWkWYtbt4pgdamlwVeZEW+LM7qZEJEsMNPrfC03APKmZsJgpWCDWOKZvkZcvjV
uYkQ4omYCTX5ohy+knMjdOmdH9c7SpqEWBDC86fiNex+O0XOMEZSa8DA
-----END CERTIFICATE-----
)EOF";

void setup_wifi() {
    while (WiFi.status() != WL_CONNECTED) {
        const char* ssid = wifi_credentials[currentWiFiIndex][0];
        const char* password = wifi_credentials[currentWiFiIndex][1];

        Serial.print("CONNECTING TO [" + String(ssid) + "]");

        WiFi.disconnect();  // Ensure a clean connection attempt
        delay(100);         // Allow disconnect to take effect
        WiFi.begin(ssid, password);

        // Wait for connection or timeout (5 seconds)
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
            delay(500);
            Serial.print(".");
        }

       if (WiFi.status() == WL_CONNECTED) {
            fetchNTPTime(); // Fetch NTP time
            Serial.println("CONNECTED TO [" + String(ssid) + "] [" + WiFi.localIP().toString() + "]");

              // Configure MQTT
              client.setServer(mqtt_server, mqtt_port);
              espClient.setCACert(root_ca); // Attach root CA certificate
              connectToTopics(); // CONNECT TO ALL TOPICS
            break; // Exit loop on successful connection
        } else {
            Serial.println("FAILED TO CONNECT TO [" + String(ssid) + "]");
            currentWiFiIndex = (currentWiFiIndex + 1) % num_wifi_credentials; // Cycle to the next SSID
        }
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("ERROR: UNABLE TO CONNECT TO ANY WIFI SSID!");
        // Handle fallback scenario if needed
    }
}

// Function to start recalibration
void startRecalibration() {
  recalibrating = true;
  recalibrationStart = millis();
  recalibrationSampleCount = 0;
  sumX = 0;
  sumY = 0;
  sumZ = 0;
  Serial.println("[RECALIBRATING BASELINE]");
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

        Serial.println("[RECALIBRATION COMPLETE] ");
        Serial.println("[X:" + String(baselineX, 2) + " Y:" + String(baselineY, 2) + " Z:" + String(baselineZ, 2) +"]");

        recalibrating = false; // End recalibration
      }
    }
  }
}

// Setup function
void setup() {
    Serial.begin(115200);
    esp_log_level_set("wifi", ESP_LOG_NONE);

    // Configure GPIO0 as output to power the MPU6050
    pinMode(MPU_POWER_PIN, OUTPUT);
    digitalWrite(MPU_POWER_PIN, HIGH); // Supply power to MPU6050
    delay(100); // Allow MPU6050 to stabilize after power-up

    // Initialize I2C with swapped SDA and SCL pins
    Wire.begin(9, 8); // SDA = GPIO9, SCL = GPIO8

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

    // Connect to Wi-Fi
    setup_wifi(); 

    // Handle MQTT keep-alive and callbacks
    client.loop();
}


// Function to print sensor data
void printSensorData(bool shockDetected, float vibrationMagnitude, sensors_event_t& accel, sensors_event_t& gyro) {
  Serial.print("|CID:"); Serial.print(thisClientID);

  Serial.print("|DD:");
  Serial.print(dateDate);

  Serial.print("|DT:");
  Serial.print(dateTime);

  Serial.print("|E:");
  Serial.print(event);

  Serial.print("|S:");
  Serial.print(shockDetected ? "DETECTED" : "LISTENING");

  Serial.print("|VM:");
  Serial.print(vibrationMagnitude, 2);

  Serial.print("|IA:");
  Serial.print(isArmed);

  Serial.print("|VT:");
  Serial.print(vibrationThreshold);

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
  unsigned long currentMillis = millis();

  // Handle MQTT keep-alive and callbacks
  client.loop();

  // Update internal clock every second
  if (currentMillis - internalClockMillis >= internalClockInterval) {
      internalClockMillis = currentMillis;
      internalEpochTime += 1; // Increment by 1 second

      // Update `dateDate` and `dateTime`
      struct tm* estTimeInfo = localtime(&internalEpochTime);
      char bufferDate[11], bufferTime[9];
      strftime(bufferDate, sizeof(bufferDate), "%m/%d", estTimeInfo);
      strftime(bufferTime, sizeof(bufferTime), "%H:%M:%S", estTimeInfo);
      dateDate = String(bufferDate);
      dateTime = String(bufferTime);
  }
  
  // Periodic Wi-Fi reconnection check and cycle
  if (currentMillis - lastWiFiCheck >= wifiCheckInterval) {
      lastWiFiCheck = currentMillis;

      // Ensure Wi-Fi is connected
      if (WiFi.status() != WL_CONNECTED) {
          Serial.println("WIFI RECONNECTING...");
          setup_wifi();  // Call your existing setup_wifi function
      }
  }

  // Refetch NTP time every 12 hours
  if (millis() - lastNTPFetchMillis >= 12UL * 60 * 60 * 1000) {
      if (WiFi.status() == WL_CONNECTED) {
          fetchNTPTime(); // Refetch NTP time
          lastNTPFetchMillis = millis(); // Reset the fetch time
      }
  }

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
    shockDetected = vibrationMagnitude > vibrationThreshold;

    if (shockDetected) {
        event = "DETECTED SHOCK";
        Serial.println("[RECALIBRATION TRIGGERED VIA SHOCK DETECTION]");
        startRecalibration(); // Start recalibration after detecting a shock
        lastShockTime = millis(); // Reset the last shock time
    }

    // Recalibrate baseline if no shock is detected for 10 minutes
    if (millis() - lastShockTime >= recalibrationInterval) {
      Serial.println("[RECALIBRATION TRIGGERED VIA 10min TIMER]");
      startRecalibration();
      lastShockTime = millis(); // Reset the last shock time after recalibration
    }

    // Print sensor data
    printSensorData(shockDetected, vibrationMagnitude, accel, gyro);
    
    // Reset flags and event after printing
    event = "-"; 
    shockDetected = false; // Explicitly reset the flag after handling
  }
}

void fetchNTPTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov"); // UTC configuration
    struct tm timeInfo;
    if (getLocalTime(&timeInfo)) {
        time_t rawTime = mktime(&timeInfo); // Get current UTC time
        internalEpochTime = rawTime; // Start with UTC

        // Determine the EST offset (-5 or -4 for DST)
        int offsetHours = -5; // Standard EST offset
        if ((timeInfo.tm_mon > 2 && timeInfo.tm_mon < 10) || // Between March and October
            (timeInfo.tm_mon == 2 && timeInfo.tm_mday >= 9) || // On or after March 9
            (timeInfo.tm_mon == 10 && timeInfo.tm_mday < 2)) { // Before November 2
            offsetHours = -4; // DST offset
        }
        internalEpochTime += offsetHours * 3600; // Apply EST offset

        // Update `dateDate` and `dateTime` with parsed EST values
        struct tm* estTimeInfo = localtime(&internalEpochTime);
        char bufferDate[11], bufferTime[9];
        strftime(bufferDate, sizeof(bufferDate), "%m/%d", estTimeInfo);
        strftime(bufferTime, sizeof(bufferTime), "%H:%M:%S", estTimeInfo);
        dateDate = String(bufferDate);
        dateTime = String(bufferTime);

        Serial.println("NTP Time Updated: " + String(dateDate) + " " + String(dateTime));
    } else {
        Serial.println("Failed to fetch NTP time");
    }
}

void connectToTopics() {
    static unsigned long lastRetryTime = 0; // Track the last retry attempt
    const unsigned long retryInterval = 5000; // Retry every 5 seconds if disconnected

    if (!client.connected()) {
        unsigned long currentMillis = millis();

        // Avoid retrying too frequently
        if (currentMillis - lastRetryTime >= retryInterval) {
            lastRetryTime = currentMillis;

            Serial.print("CONNECTING MQTT...");
            if (client.connect(clientID, mqtt_user, mqtt_password)) {
                Serial.println("CONNECTED TO: [" + String(mqtt_topic_CENTRAL_HUB) + "] [" + String(mqtt_topic_SHOCK_CENTER) + "]");

                // Subscribe to topics
                client.subscribe(mqtt_topic_CENTRAL_HUB);
                client.subscribe(mqtt_topic_SHOCK_CENTER);

                // Publish to the central hub upon successful connection
                String payload = "|CID:" + String(thisClientID) +
                                 "|DD:" + dateDate +
                                 "|DT:" + dateTime +
                                 "|E:" + event +
                                 "|IA:" + isArmed +
                                 "|VM:" + String(vibrationThreshold, 2) + // Vibration threshold or a real-time metric
                                 "|AX:" + String(baselineX, 2) +
                                 "|AY:" + String(baselineY, 2) +
                                 "|AZ:" + String(baselineZ, 2) +
                                 "|GX:" + String(sumX, 2) + // Replace `sumX`, `sumY`, `sumZ` with gyro global vars if defined
                                 "|GY:" + String(sumY, 2) +
                                 "|GZ:" + String(sumZ, 2) +
                                 "|TC:" + String(temperatureC) +
                                 "|TF:" + String(temperatureF);
                client.publish(mqtt_topic_CENTRAL_HUB, payload.c_str());
            } else {
                // Provide specific error codes for debugging
                Serial.print("failed, rc=");
                Serial.print(client.state());
                Serial.println(" try again in 5 seconds...");
            }
        }
    }
}
