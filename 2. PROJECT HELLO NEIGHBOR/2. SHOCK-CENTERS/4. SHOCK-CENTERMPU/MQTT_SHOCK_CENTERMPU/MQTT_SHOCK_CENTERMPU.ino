#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFi.h> // Add Wi-Fi library
#include "esp_log.h"
#include <WiFiClientSecure.h> // For secure SSL connection
#include <PubSubClient.h>   // For MQTT client
#include <HTTPClient.h>
#include <ArduinoJson.h>


Adafruit_MPU6050 mpu;
#define MPU_POWER_PIN 0 // GPIO0 to supply 3.3V power to MPU6050

// Global ESP variables
const char* thisClientID = "RESV-SHOCKERA"; // Define the ClientID
String isArmed = "ARMED";
String dateDate = "MM/DD";
String dateTime = "HH:MM:SS";
String event = "LISTENING";
int freeHeap = 0; // Global variable to store free heap memory

// Global Heap Variables
unsigned long lastHeapLogMillis = 0;
const unsigned long heapLogInterval = 60000; // Update every 1 minute
const int warningHeapThreshold = 15000;
const int criticalHeapThreshold = 10000;
const int emergencyHeapThreshold = 8000;

bool warningHeapFlag = false;
bool criticalHeapFlag = false;
bool emergencyHeapFlag = false;

// Global Sensor variables
float vibrationMagnitude = 0.0; // Vibration magnitude
float baselineX = 0, baselineY = 0, baselineZ = 0; // Baseline values
float gyroX = 0.0, gyroY = 0.0, gyroZ = 0.0; // variables for gyro readings
float vibrationThreshold = 0.5; // Threshold for shock detection
int temperatureC = 0; // Temperature in Celsius (whole number)
int temperatureF = 0; // Temperature in Fahrenheit (whole number)

unsigned long lastShockTime = 0; // Time of the last detected shock
const unsigned long recalibrationInterval = 600000; // 10 minutes in milliseconds
const unsigned long readAndSerialInterval = 100; // 100ms interval for sensor read and Serial output

// Vibration motor variables
#define VIBRATION_MOTOR_PIN 4 // Define the pin connected to the vibration motor
unsigned long motorToggleTime = 0;  // Tracks motor toggle time
bool motorActive = false;           // Indicates whether the motor is active
const unsigned long motorOnTime = 200; // Motor ON duration in milliseconds
const unsigned long motorOffTime = 200; // Motor OFF duration in milliseconds
const unsigned long vibrationDelay = 4000; // Delay between vibrations when armed

// NTP Variables
unsigned long lastNTPFetchMillis = 0; // Tracks the last NTP fetch time
unsigned long internalClockMillis = 0; // Tracks time since last internal clock update
const unsigned long internalClockInterval = 1000; // 1-second interval for updating the internal clock
time_t internalEpochTime = 0; // Internal clock epoch time in seconds
unsigned long lastNTPRetryMillis = 0;  // Track last retry attempt
const unsigned long ntpRetryInterval = 60000;  // Retry every 1 minute if initial fetch fails

// Variables for non-blocking recalibration
bool shockDetected = false;
bool recalibrating = false;
unsigned long recalibrationStart = 0;
unsigned long lastReadAndSerialTime = 0; // Tracks the last time sensor was read and Serial output
int recalibrationSampleCount = 0;
float sumX = 0, sumY = 0, sumZ = 0;

// GSheets Variables
#define MAX_PAYLOAD_SIZE 256
QueueHandle_t googleSheetsQueue = NULL;
TaskHandle_t googleSheetsTaskHandle = NULL;
const char* googleSheetURL = "https://script.google.com/macros/s/AKfycbzKXvDaj58CzF-1lKAhrHYrmOnBWA6omiKfFka9gppet9IrAt5zJSLdjh_7r1e9YjU-eQ/exec";

// Use Json for serial
bool useJsonForSerial = true; // Change to false for key-value

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
const char* clientID = thisClientID;
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
              client.setCallback(mqttCallback); // Set the callback here
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

        Serial.println("[RECALIBRATION COMPLETE] [X:" + String(baselineX, 2) + " Y:" + String(baselineY, 2) + " Z:" + String(baselineZ, 2) +"]");

        recalibrating = false; // End recalibration
      }
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print(String(thisClientID) + " RCVD [");
    Serial.print(topic);
    Serial.print("]: ");

    // Convert payload to a String
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);

    // Parse JSON payload
    StaticJsonDocument<512> doc; // Adjust size if necessary
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.print("Failed to parse MQTT message: ");
        Serial.println(error.c_str());
        return;
    }

    // Extract all data from JSON document
    const char* id = doc["ID"];
    const char* date = doc["DD"];
    const char* time = doc["DT"];
    const char* event = doc["E"];
    const char* isArmed = doc["IA"];
    float vibrationMagnitude = doc["VM"];
    float vibrationThreshold = doc["VT"];
    float ax = doc["AX"];
    float ay = doc["AY"];
    float az = doc["AZ"];
    float gx = doc["GX"];
    float gy = doc["GY"];
    float gz = doc["GZ"];
    int temperatureC = doc["TC"];
    int temperatureF = doc["TF"];
    int freeHeap = doc["FH"];

    // Print extracted values for debugging
    Serial.println("Extracted JSON Data:");
    Serial.println("ID: " + String(id));
    Serial.println("Date: " + String(date));
    Serial.println("Time: " + String(time));
    Serial.println("Event: " + String(event));
    Serial.println("Is Armed: " + String(isArmed));
    Serial.println("Vibration Magnitude: " + String(vibrationMagnitude, 2));
    Serial.println("Vibration Threshold: " + String(vibrationThreshold, 2));
    Serial.println("Acceleration - AX: " + String(ax, 2) + " AY: " + String(ay, 2) + " AZ: " + String(az, 2));
    Serial.println("Gyroscope - GX: " + String(gx, 2) + " GY: " + String(gy, 2) + " GZ: " + String(gz, 2));
    Serial.println("Temperature: " + String(temperatureC) + "C / " + String(temperatureF) + "F");
    Serial.println("Free Heap: " + String(freeHeap));
    
    // Example logic
    if (String(id) == "RESV-1ST") {
        Serial.println("Message is for this client!");
        if (String(event) == "ARMED") {
            isArmed = "ARMED";
            Serial.println("System is ARMED.");
        }
        if (vibrationMagnitude > vibrationThreshold) {
            Serial.println("Vibration exceeds threshold!");
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

    // Initialize vibration motor pin
    pinMode(VIBRATION_MOTOR_PIN, OUTPUT);
    digitalWrite(VIBRATION_MOTOR_PIN, LOW); // Ensure the motor is off initially

    // Initial baseline calculation
    startRecalibration();
    lastShockTime = millis(); // Record the time after the initial calibration

    // Connect to Wi-Fi
    setup_wifi(); 

    // Handle MQTT keep-alive and callbacks
    client.loop();

    // Initialize Google Sheets communication
    sheetSetup();

    event = String(thisClientID) + " CONNECTED";
    sheetAddQueue(createPayload(true)); 
    resetGlobalVariables();
}


// Function to print sensor data
void printSensorData(bool shockDetected, float vibrationMagnitude, sensors_event_t& accel, sensors_event_t& gyro) {
    if (useJsonForSerial) {
        // Use JSON format
        Serial.println(createPayload(true)); // Pass `true` to get JSON format
    } else {
        // Key-value string format
        Serial.println(createPayload(false)); // Pass `false` for key-value format
    }
}

// Main loop function
void loop() {
  unsigned long currentMillis = millis();

  // Handle MQTT keep-alive and callbacks
  client.loop();

  // Update internal clock every second
  if (currentMillis - internalClockMillis >= internalClockInterval) {
      internalClockMillis = currentMillis;

      if (internalEpochTime > 0) {
          internalEpochTime += 1; // Increment by 1 second

          // Update `dateDate` and `dateTime` with valid values
          struct tm* estTimeInfo = localtime(&internalEpochTime);
          char bufferDate[11], bufferTime[9];
          strftime(bufferDate, sizeof(bufferDate), "%m/%d", estTimeInfo);
          strftime(bufferTime, sizeof(bufferTime), "%H:%M:%S", estTimeInfo);
          dateDate = String(bufferDate);
          dateTime = String(bufferTime);
      } else {
          // NTP has not synced yet
          dateDate = "N/A";
          dateTime = "N/A";
      }
  }
  
  // Periodic Wi-Fi reconnection check and cycle
  if (currentMillis - lastWiFiCheck >= wifiCheckInterval) {
      lastWiFiCheck = currentMillis;

      // Ensure Wi-Fi is connected
      if (WiFi.status() != WL_CONNECTED) {
          Serial.println("WIFI RECONNECTING...");
          setup_wifi();  // Call your existing setup_wifi function
          event = String(thisClientID) + " CONNECTED";
          sheetAddQueue(createPayload(true));
          resetGlobalVariables(); 
      }
  }

  // Retry NTP fetch if needed
    if (WiFi.status() == WL_CONNECTED) {
        if (internalEpochTime == 0 || millis() - lastNTPRetryMillis >= ntpRetryInterval) {
            lastNTPRetryMillis = millis();  // Update retry timestamp
            if (fetchNTPTime()) {
                lastNTPFetchMillis = millis();  // Reset 12-hour fetch timer
            }
        }
    }

  // Refetch NTP time every 12 hours
  if (millis() - lastNTPFetchMillis >= 12UL * 60 * 60 * 1000) {
      if (WiFi.status() == WL_CONNECTED) {
          fetchNTPTime(); // Refetch NTP time
          lastNTPFetchMillis = millis(); // Reset the fetch time
      }
  }

  // Call logFreeHeap periodically
  if (millis() - lastHeapLogMillis >= heapLogInterval) {
      lastHeapLogMillis = millis();
      logFreeHeap();
  }

  // Handle recalibration logic
  handleRecalibration();

  // Handle motor toggling for vibration motor (non-blocking)
  handleVibrationMotor();

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

    // Update gyro global variables
    gyroX = gyro.gyro.x;
    gyroY = gyro.gyro.y;
    gyroZ = gyro.gyro.z;

    // Calculate vibration deviation from baseline
    float deviationX = accel.acceleration.x - baselineX;
    float deviationY = accel.acceleration.y - baselineY;
    float deviationZ = accel.acceleration.z - baselineZ;

    // Calculate vibration magnitude
    vibrationMagnitude = sqrt(pow(deviationX, 2) + pow(deviationY, 2) + pow(deviationZ, 2));


    // Print sensor data
    printSensorData(shockDetected, vibrationMagnitude, accel, gyro);

    // If the vibration magnitude exceeds the threshold, a shock is detected
    shockDetected = vibrationMagnitude > vibrationThreshold;

    if (shockDetected) {
        event = "DETECTED SHOCK";
        // Publish to Central Hub
        publishMQTT();
        sheetAddQueue(createPayload(true)); // Pass `true` for JSON payload

        Serial.println("[RECALIBRATION TRIGGERED VIA SHOCK DETECTION]");
        printSensorData(shockDetected, vibrationMagnitude, accel, gyro);

        if (isArmed == "ARMED") {
                motorActive = true; // Activate the motor
                motorToggleTime = currentMillis; // Set the motor toggle time
                digitalWrite(VIBRATION_MOTOR_PIN, HIGH); // Start the motor
            }

        startRecalibration(); // Start recalibration after detecting a shock
        lastShockTime = millis(); // Reset the last shock time
        
        resetGlobalVariables();
    }

    // Recalibrate baseline if no shock is detected for 10 minutes
    if (millis() - lastShockTime >= recalibrationInterval) {
      event = "RECALIBRATING";
      Serial.println("[RECALIBRATION TRIGGERED VIA 10min TIMER]");
      printSensorData(shockDetected, vibrationMagnitude, accel, gyro);

      startRecalibration();
      lastShockTime = millis(); // Reset the last shock time after recalibration
      resetGlobalVariables();
    }
  }
}

bool fetchNTPTime() {
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

        return true;  // Fetch successful
    } else {
        Serial.println("Failed to fetch NTP time");
        return false;  // Fetch failed
    }
}

void resetGlobalVariables() {
    event = "LISTENING";
    shockDetected = false;
    vibrationMagnitude = 0.0;
    baselineX = 0;
    baselineY = 0;
    baselineZ = 0;
    gyroX = 0.0;
    gyroY = 0.0;
    gyroZ = 0.0;
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

                event = String(thisClientID) + " CONNECTED";

                // Publish to the central hub upon successful connection
                publishMQTT();
                resetGlobalVariables(); // Reset global variables after successful connection
            } else {
                // Provide specific error codes for debugging
                Serial.print("failed, rc=");
                Serial.print(client.state());
                Serial.println(" try again in 5 seconds...");
            }
        }
    }
}

// Handles Serial, If bool is true, handles MQTT and Sheets payload
String createPayload(bool forJson) {
    String payload;

    if (forJson) {
        // JSON format for MQTT and Google Sheets
        payload = "{";
        payload += "\"ID\":\"" + String(thisClientID) + "\",";
        payload += "\"DD\":\"" + dateDate + "\",";
        payload += "\"DT\":\"" + dateTime + "\",";
        payload += "\"E\":\"" + event + "\",";
        payload += "\"IA\":\"" + isArmed + "\",";
        payload += "\"VM\":" + String(vibrationMagnitude, 2) + ",";
        payload += "\"VT\":" + String(vibrationThreshold, 2) + ",";
        payload += "\"AX\":" + String(baselineX, 2) + ",";
        payload += "\"AY\":" + String(baselineY, 2) + ",";
        payload += "\"AZ\":" + String(baselineZ, 2) + ",";
        payload += "\"GX\":" + String(gyroX, 2) + ",";
        payload += "\"GY\":" + String(gyroY, 2) + ",";
        payload += "\"GZ\":" + String(gyroZ, 2) + ",";
        payload += "\"TC\":" + String(temperatureC) + ",";
        payload += "\"TF\":" + String(temperatureF) + ",";
        payload += "\"FH\":" + String(freeHeap); // Add free heap
        payload += "}";
    } else {
        // Key-value string format for Serial
        payload = "|ID:" + String(thisClientID) +
                  "|DD:" + dateDate +
                  "|DT:" + dateTime +
                  "|E:" + event +
                  "|IA:" + isArmed +
                  "|VM:" + String(vibrationMagnitude, 2) +
                  "|VT:" + String(vibrationThreshold, 2) +
                  "|AX:" + String(baselineX, 2) +
                  "|AY:" + String(baselineY, 2) +
                  "|AZ:" + String(baselineZ, 2) +
                  "|GX:" + String(gyroX, 2) +
                  "|GY:" + String(gyroY, 2) +
                  "|GZ:" + String(gyroZ, 2) +
                  "|TC:" + String(temperatureC) +
                  "|TF:" + String(temperatureF) +
                  "|FH:" + String(freeHeap); // Add free heap
    }

    return payload;
}

void publishMQTT() {
    if (WiFi.status() == WL_CONNECTED && client.connected()) {
        client.publish(mqtt_topic_CENTRAL_HUB, createPayload(true).c_str()); // Pass `true` for JSON payload
    } else {
        Serial.println("Error: Cannot publish detection - WiFi/MQTT not connected.");
    }
}

// publish connected

// publish confrim

// Function to handle motor toggling behavior
void handleVibrationMotor() {
    unsigned long currentMillis = millis();

    if (motorActive) {
        // Toggle motor state
        if ((currentMillis - motorToggleTime >= motorOnTime) && digitalRead(VIBRATION_MOTOR_PIN) == HIGH) {
            digitalWrite(VIBRATION_MOTOR_PIN, LOW); // Turn motor off
            motorToggleTime = currentMillis;
        } else if ((currentMillis - motorToggleTime >= motorOffTime) && digitalRead(VIBRATION_MOTOR_PIN) == LOW) {
            digitalWrite(VIBRATION_MOTOR_PIN, HIGH); // Turn motor on
            motorToggleTime = currentMillis;
        }

        // Stop the motor after the vibration delay
        if (currentMillis - lastShockTime >= vibrationDelay) {
            motorActive = false;
            digitalWrite(VIBRATION_MOTOR_PIN, LOW); // Ensure the motor is off
        }
    }
}

void sheetSetup() {
    // Create queue for Google Sheets communication
    googleSheetsQueue = xQueueCreate(10, MAX_PAYLOAD_SIZE);
    if (googleSheetsQueue == NULL) {
        Serial.println("ERROR: Failed to create queue for Google Sheets.");
    }

    // Create RTOS task for Google Sheets
    xTaskCreate(
        sheetTask,                 // Task function
        "GoogleSheetsTask",        // Task name
        4096,                      // Stack size
        NULL,                      // Task parameter
        1,                         // Task priority
        &googleSheetsTaskHandle    // Task handle
    );

    if (googleSheetsTaskHandle == NULL) {
        Serial.println("ERROR: Failed to create Google Sheets task.");
    }
}

// RTOS task to handle Google Sheets communication
void sheetTask(void* parameter) {
    char payload[MAX_PAYLOAD_SIZE];
    while (true) {
        // Wait for data in the queue
        if (xQueueReceive(googleSheetsQueue, &payload, portMAX_DELAY)) {
            Serial.println("INFO: Received payload for Google Sheets:");
            Serial.println(payload);

            if (WiFi.status() == WL_CONNECTED) {
                HTTPClient http;
                http.begin(googleSheetURL);
                http.addHeader("Content-Type", "application/json");

                int httpResponseCode = http.POST(payload);

                if (httpResponseCode > 0) {
                    Serial.println("POST RESPONSE CODE: " + String(httpResponseCode));
                } else {
                    Serial.println("ERROR: Failed to send payload to Google Sheets.");
                }

                http.end();
            } else {
                Serial.println("ERROR: Wi-Fi not connected. Unable to send to Google Sheets.");
            }
        }
    }
}

// Function to enqueue payloads for Google Sheets
void sheetAddQueue(const String& payload) {
    if (payload.length() >= MAX_PAYLOAD_SIZE) {
        Serial.println("ERROR: Payload exceeds maximum size.");
        return;
    }

    if (xQueueSend(googleSheetsQueue, payload.c_str(), portMAX_DELAY) != pdPASS) {
        Serial.println("ERROR: Failed to enqueue payload to Google Sheets.");
    } else {
        Serial.println("INFO: Payload enqueued for Google Sheets.");
    }
}

void logFreeHeap() {
    freeHeap = ESP.getFreeHeap();

    // WARNING: Heap < 15,000 bytes
    if (freeHeap < warningHeapThreshold && !warningHeapFlag) {
        warningHeapFlag = true;
        Serial.println("[HEAP WARNING] " + String(thisClientID) + " BELOW WARNING THRESHOLD");
        event = "[HEAP WARNING] " + String(thisClientID) + " BELOW WARNING THRESHOLD";
        publishMQTT();
        sheetAddQueue(createPayload(true));
    }
    if (freeHeap >= warningHeapThreshold && warningHeapFlag) {
        warningHeapFlag = false; // Reset flag when heap recovers
        Serial.println("[HEAP WARNING] " + String(thisClientID) + " RECOVERED TO STABLE");
        event = "[HEAP WARNING] " + String(thisClientID) + " RECOVERED TO STABLE"; // Update event for recovery
        publishMQTT();
        sheetAddQueue(createPayload(true));
        event = "LISTENING";
    }

    // CRITICAL: Heap < 10,000 bytes
    if (freeHeap < criticalHeapThreshold) {
        if (!criticalHeapFlag) {
            criticalHeapFlag = true;
            Serial.println("[HEAP CRITICAL] " + String(thisClientID) + " BELOW CRITICAL THRESHOLD");
            event = "[HEAP CRITICAL] " + String(thisClientID) + " BELOW CRITICAL THRESHOLD";
            publishMQTT();
            sheetAddQueue(createPayload(true));
        }
    } else if (freeHeap >= criticalHeapThreshold) {
        if (criticalHeapFlag) {
            criticalHeapFlag = false; // Reset flag when heap recovers
            Serial.println("[HEAP CRITICAL] " + String(thisClientID) + " RECOVERED TO WARNING");
            event = "[HEAP CRITICAL] " + String(thisClientID) + " RECOVERED TO WARNING"; // Update event for recovery
            publishMQTT();
            sheetAddQueue(createPayload(true));
        }
    }

    // EMERGENCY: Heap < 8,000 bytes
    if (freeHeap < emergencyHeapThreshold) {
        if (!emergencyHeapFlag) {
            emergencyHeapFlag = true;
            Serial.println("[HEAP EMERGENCY] " + String(thisClientID) + " BELOW EMERGENCY THRESHOLD, REBOOTING");
            event = "[HEAP EMERGENCY] " + String(thisClientID) + " BELOW EMERGENCY THRESHOLD, REBOOTING";
            publishMQTT();
            sheetAddQueue(createPayload(true));
            delay(10000); // Give time for MQTT and Sheets data to send
            ESP.restart(); // Reboot the ESP32
        }
    } else if (freeHeap >= emergencyHeapThreshold) {
        if (emergencyHeapFlag) {
            emergencyHeapFlag = false; // Reset flag when heap recovers
            Serial.println("[HEAP EMERGENCY] " + String(thisClientID) + " RECOVERED TO CRITICAL");
            event = "[HEAP EMERGENCY] " + String(thisClientID) + " RECOVERED TO CRITICAL"; // Update event for recovery
            publishMQTT();
            sheetAddQueue(createPayload(true));
        }
    }
}
