#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFi.h> // Add Wi-Fi library
#include "esp_log.h"
#include <WiFiClientSecure.h> // For secure SSL connection
#include <PubSubClient.h>   // For MQTT client
#include <HTTPClient.h>
#include <ArduinoJson.h>
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
#define LCD_BLK 32

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

// Basic Colors
uint16_t BLACK   = lcd.color565(0, 0, 0);
uint16_t BLUE    = lcd.color565(0, 0, 255);
uint16_t RED     = lcd.color565(255, 0, 0);
uint16_t GREEN   = lcd.color565(0, 255, 0);
uint16_t CYAN    = lcd.color565(0, 255, 255);
uint16_t MAGENTA = lcd.color565(255, 0, 255);
uint16_t YELLOW  = lcd.color565(255, 255, 0);
uint16_t WHITE   = lcd.color565(255, 255, 255);
// Additional Colors
uint16_t LIGHT_BLUE = lcd.color565(100, 150, 255);
uint16_t ORANGE  = lcd.color565(255, 165, 0);
uint16_t PINK    = lcd.color565(255, 105, 180);
uint16_t PURPLE  = lcd.color565(128, 0, 128);
uint16_t TAN        = lcd.color565(210, 180, 140);
uint16_t NAVY       = lcd.color565(0, 0, 128);
uint16_t BURGUNDY = lcd.color565(128, 0, 32);
uint16_t DEEP_PURPLE = lcd.color565(75, 0, 130);
uint16_t GRAY = lcd.color565(128, 128, 128);

// Global Variables for Button Handling
#define EXTERNAL_BUTTON_PIN 22  // GPIO pin for the external button
#define ONBOARD_BUTTON_PIN 0   // GPIO pin for the onboard button
unsigned long buttonPressStart = 0; // Track when the button was pressed
bool isButtonHeld = false;         // Track if the button is being held
bool buttonPressed = false;        // Track button state
unsigned long debounceDelay = 50;  // Debounce delay
unsigned long lastButtonPressTime = 0;
unsigned long buttonHoldDurationThreshold = 1500; // Button duration threshold (in milliseconds) 1.5 seconds
float vtStep = 0.01; // Stride or size used to adjust Vibration Threshold

// Global ESP variables
const char* thisClientID = "RESV-1ST"; // Define the ClientID
String isArmed = " --";
String dateDate = "MM/DD";
String dateTime = "HH:MM:SS";
String event = "LISTENING";
int freeHeap = 0; // Global variable to store free heap memory

int retaliationCount = 0;
String retaliationTime = "00:00:00"; // To store the formatted time
bool bootTimeCaptured = false; // Ensure boot time is only captured once
String bootDate = "MM/DD";
String bootTime = "HH/MM";

#define MAX_LOGS 20 // Maximum number of log entries to display
String logTextBuffer[MAX_LOGS]; // Buffer for log text
uint16_t logColorBuffer[MAX_LOGS]; // Buffer for log colors
int logCount = 0; // Current number of logs

// Global Heap Variables
unsigned long lastHeapLogMillis = 0;
const unsigned long heapLogInterval = 60000; // Update every 1 minute
const int warningHeapThreshold = 15000;
const int criticalHeapThreshold = 10000;
const int emergencyHeapThreshold = 8000;

bool warningHeapFlag = false;
bool criticalHeapFlag = false;
bool emergencyHeapFlag = false;

String ID = "";

// Global Sensor variables
float vibrationMagnitude = 0.0; // Vibration magnitude
float vibrationThreshold = .50; // Threshold for shock detection
float baselineX = 0, baselineY = 0, baselineZ = 0; // Baseline values
float gyroX = 0.0, gyroY = 0.0, gyroZ = 0.0; // variables for gyro readings

int temperatureC = 0; // Temperature in Celsius (whole number)
int temperatureF = 0; // Temperature in Fahrenheit (whole number)


// NTP Variables
unsigned long lastNTPFetchMillis = 0; // Tracks the last NTP fetch time
unsigned long internalClockMillis = 0; // Tracks time since last internal clock update
const unsigned long internalClockInterval = 1000; // 1-second interval for updating the internal clock
time_t internalEpochTime = 0; // Internal clock epoch time in seconds
unsigned long lastNTPRetryMillis = 0;  // Track last retry attempt
const unsigned long ntpRetryInterval = 60000;  // Retry every 1 minute if initial fetch fails


// GSheets Variables
#define MAX_PAYLOAD_SIZE 256

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

        // Set the cursor for the first line (fixed position)
        lcd.fillScreen(ST77XX_BLACK);
        lcd.setCursor(25, 0);
        lcd.println("WIFI Connecting");

        // Calculate width of the SSID string
        lcd.setCursor(80, 28);
        lcd.println("[" + String(ssid) + "]");

        WiFi.disconnect();  // Ensure a clean connection attempt
        delay(100);         // Allow disconnect to take effect
        WiFi.begin(ssid, password);

        // Wait for connection or timeout (5 seconds)
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
          delay(500);
          Serial.print(".");
          lcd.print(".");
        }

       if (WiFi.status() == WL_CONNECTED) {
          fetchNTPTime(); // Fetch NTP time
          Serial.println("CONNECTED TO [" + String(ssid) + "] [" + WiFi.localIP().toString() + "]");

            lcd.fillScreen(ST77XX_BLACK);
            lcd.setCursor(50, 0);
            lcd.println("CONNECTED TO");
            lcd.setCursor(80, 28);
            lcd.setTextColor(ST77XX_GREEN);
            lcd.println("[" + String(ssid) + "]");
            lcd.setCursor(30, 56);
            lcd.println("[" + WiFi.localIP().toString() + "]");
            

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
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
      Serial.print("Failed to parse MQTT message: ");
      Serial.println(error.c_str());
      return;
    }

    // Extract data into temporary `receivedX` variables
    const char* receivedId = doc["ID"];
    const char* receivedDate = doc["DD"];
    const char* receivedTime = doc["DT"];
    const char* receivedEvent = doc["E"];
    const char* receivedIsArmed = doc["IA"];
    float receivedVibrationMagnitude = doc["VM"];
    float receivedVibrationThreshold = doc["VT"];
    float receivedAx = doc["AX"];
    float receivedAy = doc["AY"];
    float receivedAz = doc["AZ"];
    float receivedGx = doc["GX"];
    float receivedGy = doc["GY"];
    float receivedGz = doc["GZ"];
    int receivedTemperatureC = doc["TC"];
    int receivedTemperatureF = doc["TF"];
    int receivedFreeHeap = doc["FH"];

    // Check if all mandatory fields are present
    if (!receivedId || !receivedEvent) {
      Serial.println("CRITICAL JSON FIELDS MISSING, IGNORING MESSAGE...");
      Serial.println("ORIGINAL MESSAGE: " + message); // Log the original payload
      event = String(thisClientID) + "RCVD CRITICAL JSON FIELDS MISSING, IGNORING MESSAGE...";
      resetGlobalVariables();
      return;
    }
    
    // Shorten ID and Event
    if (receivedId) {
      ID = (String(receivedId) == "SHOCK-A") ? "A" :
        (String(receivedId) == "SHOCK-B") ? "B" :
        (String(receivedId) == "SHOCK-C") ? "C" :
        (String(receivedId) == "RESV-1ST") ? "1" :
        (String(receivedId) == "RESV-2ND") ? "2" :
        (String(receivedId) == "RESV-3RD") ? "3" :
        String(receivedId); // Default to the original ID if no match
    } else {
        ID = "UNKNOWN";
    }

    // Normalize Event
    if (receivedEvent) {
      String smallEvent = String(receivedEvent);  // Create a local copy for event processing

      if (smallEvent.indexOf("DETECTED") != -1) {
          event = "DETECTED";  // Update the global `event` variable
      } else if (smallEvent.indexOf("CONFIRMED") != -1) {
          event = "CONFIRMED";  // Simplified logic
      } else if (smallEvent.indexOf("REQUESTED") != -1) {
          event = "REQUESTED";
      } else if (smallEvent.indexOf("ADJUSTED") != -1) {
          event = "ADJUSTED";
      } else if (smallEvent.indexOf("INCREASED") != -1) {
          event = "INCREASED";
      } else if (smallEvent.indexOf("DECREASED") != -1) {
          event = "DECREASED";
      } else if (smallEvent.indexOf("CONNECTED") != -1) {
          event = "CONNECTED";
      } else {
          event = "UNKNOWN EVENT";  // Default fallback
      }
    } else {
      event = "UNKNOWN";  // If `receivedEvent` is null or empty
    }

    // Debug: Print updated global state and received values
    Serial.println("-- GLOBAL VARIABLES BEFORE EXPLICIT NEW VALUES / RECEIVED VALUES ---");
    Serial.println("ID: " + ID + " | receivedId: " + String(receivedId) + " (converted receivedID will be shortened to StringLetter)");
    Serial.println("Date: " + dateDate + " | receivedDate: " + String(receivedDate));
    Serial.println("Time: " + dateTime + " | receivedTime: " + String(receivedTime));
    Serial.println("Event: " + event + " | receivedEvent: " + String(receivedEvent));
    Serial.println("IsArmed: " + isArmed + " | receivedIsArmed: " + String(receivedIsArmed));
    Serial.println("Vibration Magnitude: " + String(vibrationMagnitude, 2) + " | receivedVibrationMagnitude: " + String(receivedVibrationMagnitude, 2));
    Serial.println("Vibration Threshold: " + String(vibrationThreshold, 2) + " | receivedVibrationThreshold: " + String(receivedVibrationThreshold, 2));
    Serial.println("Acceleration - AX: " + String(baselineX, 2) + " AY: " + String(baselineY, 2) + " AZ: " + String(baselineZ, 2) +
                    " | receivedAx: " + String(receivedAx, 2) + " Ay: " + String(receivedAy, 2) + " Az: " + String(receivedAz, 2));
    Serial.println("Gyroscope - GX: " + String(gyroX, 2) + " GY: " + String(gyroY, 2) + " GZ: " + String(gyroZ, 2) +
                    " | receivedGx: " + String(receivedGx, 2) + " Gy: " + String(receivedGy, 2) + " Gz: " + String(receivedGz, 2));
    Serial.println("Temperature: " + String(temperatureC) + "C / " + String(temperatureF) + "F" +
                    " | receivedTemperatureC: " + String(receivedTemperatureC) + "C / " + String(receivedTemperatureF) + "F");
    Serial.println("Free Heap: " + String(freeHeap) + " | receivedFreeHeap: " + String(receivedFreeHeap));

    // EXAMPLE OF LOCAL TO GLOBAL ASSIGNMENTS 
    // ID = String(receivedId);
    // dateDate = String(receivedDate);
    // dateTime = String(receivedTime);
    // event = String(receivedEvent);
    // isArmed = String(receivedIsArmed);
    // vibrationMagnitude = receivedVibrationMagnitude;
    // vibrationThreshold = receivedVibrationThreshold;
    // baselineX = receivedAx;
    // baselineY = receivedAy;
    // baselineZ = receivedAz;
    // gyroX = receivedGx;
    // gyroY = receivedGy;
    // gyroZ = receivedGz;
    // temperatureC = receivedTemperatureC;
    // temperatureF = receivedTemperatureF;
    // freeHeap = receivedFreeHeap;

    //CALL BACK FOR REQUEST AND NOT THIS CLIENT ID, WE CAPTURE OFFICIAL END TO END COMMS
    if (String(receivedEvent).indexOf("REQUESTED") != -1 && String(receivedId) == thisClientID) {
      LCDUpdateLog();
      resetGlobalVariables();
    }

    // CALLBACK FOR EXPLICIT OR GLOBAL ARM/DISARM REQUESTS, ASSIGNVALUE, SEND CONFIRMATION
    if (String(receivedEvent).indexOf("CONFIRMED") != -1) {
      // Extract state from receivedEvent if needed
      isArmed = receivedIsArmed; // Only if separate logic needs it
      vibrationMagnitude = receivedVibrationMagnitude;
      temperatureC = receivedTemperatureC;
      temperatureF = receivedTemperatureF;
      LCDClearZone(ID);
      LCDUpdateZone(ID);
      LCDUpdateLog();
      resetGlobalVariables();
    }

    // CALLBACK FOR CONNECTED AND NOT THIS CLIENT ID, WE CAPTURE OFFICIAL END TO END
    if (String(receivedEvent).indexOf("CONNECTED") != -1 && String(receivedId) != thisClientID) {
      isArmed = receivedIsArmed; // Only if separate logic needs it
      vibrationMagnitude = receivedVibrationMagnitude;
      temperatureC = receivedTemperatureC;
      temperatureF = receivedTemperatureF;
      LCDUpdateLog();
      resetGlobalVariables();
    }

    // CALLBACK FOR DETECTED AND NOT THIS CLIENT ID, WE CAPTURE OFFICIAL END TO END
    if (String(receivedEvent).indexOf("DETECTED") != -1 && String(receivedId) != thisClientID) {
      isArmed = receivedIsArmed; // Only if separate logic needs it
      vibrationMagnitude = receivedVibrationMagnitude;
      temperatureC = receivedTemperatureC;
      temperatureF = receivedTemperatureF;
      LCDClearZone(ID);
      LCDUpdateZone(ID);
      LCDUpdateLog();
      LCDUpdateHeader();
      resetGlobalVariables();
    }

    // CALL BACK FOR ADJUSTED WE CAPTURE OFFICIAL END TO END COMMS
    if (String(receivedEvent).indexOf("ADJUSTED") != -1 && String(receivedId) != thisClientID) {
      isArmed = receivedIsArmed; // Only if separate logic needs it
      vibrationMagnitude = receivedVibrationMagnitude;
      vibrationThreshold = receivedVibrationThreshold;
      temperatureC = receivedTemperatureC;
      temperatureF = receivedTemperatureF;
      LCDUpdateLog();
      resetGlobalVariables();
    }

    // CALL BACK FOR INCREASED WE CAPTURE OFFICIAL END TO END COMMS
    if (String(receivedEvent).indexOf("INCREASED") != -1 && String(receivedId) != thisClientID) {
      isArmed = receivedIsArmed; // Only if separate logic needs it
      vibrationMagnitude = receivedVibrationMagnitude;
      vibrationThreshold = receivedVibrationThreshold;
      temperatureC = receivedTemperatureC;
      temperatureF = receivedTemperatureF;
      LCDClearZone(ID);
      LCDUpdateZone(ID);
      LCDUpdateLog();
      resetGlobalVariables();
    }

    // CALL BACK FOR DECREASED WE CAPTURE OFFICIAL END TO END COMMS
    if (String(receivedEvent).indexOf("DECREASED") != -1 && String(receivedId) != thisClientID) {
      isArmed = receivedIsArmed; // Only if separate logic needs it
      vibrationMagnitude = receivedVibrationMagnitude;
      vibrationThreshold = receivedVibrationThreshold;
      temperatureC = receivedTemperatureC;
      temperatureF = receivedTemperatureF;
      LCDClearZone(ID);
      LCDUpdateZone(ID);
      LCDUpdateLog();
      resetGlobalVariables();
    }
}


// Setup function
void setup() {
    Serial.begin(115200);
    esp_log_level_set("wifi", ESP_LOG_NONE);

    //Button setup
    pinMode(ONBOARD_BUTTON_PIN, INPUT_PULLUP); // Ensure the button pin is set to INPUT_PULLUP

    LCDInitialize();

    // Connect to Wi-Fi
    setup_wifi(); 

    // Handle MQTT keep-alive and callbacks
    client.loop();

    resetGlobalVariables();

    LCDDashboard();
}

// Main loop function
void loop() {
  unsigned long currentMillis = millis();

  // Handle MQTT keep-alive and callbacks
  client.loop();

  // Button press logic
  bool currentButtonState = digitalRead(ONBOARD_BUTTON_PIN) == LOW; // Active LOW
  if (currentButtonState && !buttonPressed) {
      // Button just pressed
      buttonPressed = true;
      buttonPressStart = currentMillis;
      isButtonHeld = false; // Reset the long-press flag
  }

  if (buttonPressed && currentButtonState) {
      // Button is being held down
      unsigned long pressDuration = currentMillis - buttonPressStart;
      if (pressDuration >= buttonHoldDurationThreshold && !isButtonHeld) {
          // Long press detected: Publish global request
          isButtonHeld = true; // Prevent multiple triggers
          publishArmDisarmEvent(true);
          //publishAdjustVibrationThreshold(vtStep, true);
          //publishAdjustVibrationThreshold(-vtStep, true);
      }
  }

  if (!currentButtonState && buttonPressed) {
      // Button released
      buttonPressed = false;
      unsigned long pressDuration = currentMillis - buttonPressStart;

      if (pressDuration > 0 && pressDuration < buttonHoldDurationThreshold) {
          // Short press detected: Publish targeted request
          publishArmDisarmEvent(false);
          //publishAdjustVibrationThreshold(vtStep, false);
          //publishAdjustVibrationThreshold(-vtStep, false);
      }
  }


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

        // Capture boot time if not already set
        if (!bootTimeCaptured) {
            bootDate = dateDate; // Set static boot date

            // Reformat bufferTime to include only HH:MM
            strftime(bufferTime, sizeof(bufferTime), "%H:%M", estTimeInfo);

            bootTime = String(bufferTime); // Set static boot time
            bootTimeCaptured = true; // Mark as captured
        }

        return true;  // Fetch successful
    } else {
        Serial.println("Failed to fetch NTP time");
        return false;  // Fetch failed
    }
}

void resetGlobalVariables() {
    ID = "";
    event = "LISTENING";
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

void logFreeHeap() {
    freeHeap = ESP.getFreeHeap();

    // WARNING: Heap < 15,000 bytes
    if (freeHeap < warningHeapThreshold && !warningHeapFlag) {
        warningHeapFlag = true;
        Serial.println("[HEAP WARNING] " + String(thisClientID) + " BELOW WARNING THRESHOLD");
        event = "[HEAP WARNING] " + String(thisClientID) + " BELOW WARNING THRESHOLD";
        publishMQTT();
    }
    if (freeHeap >= warningHeapThreshold && warningHeapFlag) {
        warningHeapFlag = false; // Reset flag when heap recovers
        Serial.println("[HEAP WARNING] " + String(thisClientID) + " RECOVERED TO STABLE");
        event = "[HEAP WARNING] " + String(thisClientID) + " RECOVERED TO STABLE"; // Update event for recovery
        publishMQTT();
        event = "LISTENING";
    }

    // CRITICAL: Heap < 10,000 bytes
    if (freeHeap < criticalHeapThreshold) {
        if (!criticalHeapFlag) {
            criticalHeapFlag = true;
            Serial.println("[HEAP CRITICAL] " + String(thisClientID) + " BELOW CRITICAL THRESHOLD");
            event = "[HEAP CRITICAL] " + String(thisClientID) + " BELOW CRITICAL THRESHOLD";
            publishMQTT();
        }
    } else if (freeHeap >= criticalHeapThreshold) {
        if (criticalHeapFlag) {
            criticalHeapFlag = false; // Reset flag when heap recovers
            Serial.println("[HEAP CRITICAL] " + String(thisClientID) + " RECOVERED TO WARNING");
            event = "[HEAP CRITICAL] " + String(thisClientID) + " RECOVERED TO WARNING"; // Update event for recovery
            publishMQTT();
        }
    }

    // EMERGENCY: Heap < 8,000 bytes
    if (freeHeap < emergencyHeapThreshold) {
        if (!emergencyHeapFlag) {
            emergencyHeapFlag = true;
            Serial.println("[HEAP EMERGENCY] " + String(thisClientID) + " BELOW EMERGENCY THRESHOLD, REBOOTING");
            event = "[HEAP EMERGENCY] " + String(thisClientID) + " BELOW EMERGENCY THRESHOLD, REBOOTING";
            publishMQTT();
            delay(10000); // Give time for MQTT and Sheets data to send
            ESP.restart(); // Reboot the ESP32
        }
    } else if (freeHeap >= emergencyHeapThreshold) {
        if (emergencyHeapFlag) {
            emergencyHeapFlag = false; // Reset flag when heap recovers
            Serial.println("[HEAP EMERGENCY] " + String(thisClientID) + " RECOVERED TO CRITICAL");
            event = "[HEAP EMERGENCY] " + String(thisClientID) + " RECOVERED TO CRITICAL"; // Update event for recovery
            publishMQTT();
            
        }
    }
}

void publishArmDisarmEvent(bool isGlobal) {
    // Toggle the armed state
    isArmed = (isArmed == "DISARMED") ? "ARMED" : "DISARMED";

    // Construct event string based on the type of request
    if (isGlobal) {
        event = String(thisClientID) + " REQUESTED " + isArmed + " TO #";
    } else {
        event = String(thisClientID) + " REQUESTED " + isArmed + " TO SHOCK-A";
    }

    // Publish the event
    publishMQTT();
}

void publishAdjustVibrationThreshold(float adjustment, bool isGlobal) {
    vibrationThreshold += adjustment;

    // Prevent vibrationThreshold from going below 0
    if (vibrationThreshold < 0.0) {
        vibrationThreshold = 0.0;
        Serial.println("Vibration threshold cannot be negative. Reset to 0.");
    }

    // Determine direction and update XYZ dynamically
    String direction = (adjustment > 0) ? "+" : "-";
  
    // Construct the event string
    event = String(thisClientID) + " ADJUSTED VIBRATION THRESHOLD BY " + direction + abs(adjustment) +  
            (isGlobal ? " TO #" : " TO SHOCK-A");

    // Publish the event
    publishMQTT();
    resetGlobalVariables();
}


void LCDInitialize(){
  lcd.init(LCD_WIDTH, LCD_HEIGHT);
  lcd.setRotation(3);
  lcd.fillScreen(ST77XX_BLACK);
  lcd.setTextColor(ST77XX_WHITE);
  lcd.setTextSize(3);
  lcd.setCursor(0, 0);
}

void LCDClearHeader(){
  lcd.fillRect(0, 0, 259, 24, BLACK); // Assumed font size of 3.
}

void LCDClearLog(){
  lcd.fillRect(0, 24, 320, 80, BLACK); // Assumed font size of 3.
}

//pass ID
void LCDClearZone(String zone) {
    // Handle default value for the parameter
    if (zone == "") zone = "ALL";

    // Clear all zones if "ALL" is specified
    if (zone == "ALL") {
        LCDClearZone("A");
        LCDClearZone("B");
        LCDClearZone("C");
        return;
    }

    // Set default xStart and width for SHOCK-A
    int xStart = 0;
    int width = 105;

    // Determine xStart and width based on the zone
    if (zone == "B") {
        xStart = 107;
        width = 106;
    } else if (zone == "C") {
        xStart = 215;
        width = 105;
    } else if (zone != "A") {
        Serial.println("Invalid zone specified!");
        return;
    }

    // Clear the entire zone
    lcd.fillRect(xStart, 109, width, 170 - 109, BLACK); // Clear the entire zone
}


// Unified function to increment count, calculate time, format it, and print
void LCDUpdateHeader(){
    LCDClearHeader();

  //Count and Time Conversion
    // Increment the retaliation count
    retaliationCount++;

    // Calculate total seconds for the retaliation time
    int totalSeconds = 4 * retaliationCount;

    // Format the total seconds into HH:MM:SS
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    char timeBuffer[9];
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", hours, minutes, seconds);

    // Update the global retaliation time
    retaliationTime = String(timeBuffer);

    lcd.setTextColor(YELLOW);
    lcd.setTextSize(2);
    lcd.setCursor(0, 0);
    lcd.print("#");
    lcd.setTextSize(3);
    lcd.print(String(retaliationCount));
    lcd.setCursor(115, 0);
    lcd.print(String(retaliationTime)); 
}

void LCDDashboard(){
    lcd.init(LCD_WIDTH, LCD_HEIGHT);
    lcd.fillScreen(ST77XX_BLACK);
    lcd.setRotation(3);

    // Bottom Grid Draw lines for 3 equal columns
    lcd.drawLine(106, 109, 106, 170, YELLOW); // 1st Vertical line, 1 & 2
    lcd.drawLine(214, 109, 214, 170, YELLOW); // 2nd Vertical line, 2 & 3
    // Horizontal line across the screen at y = 108
    lcd.drawLine(0, 105, 320, 105, YELLOW); // Single Horizontal line across the screen

    lcd.setTextColor(YELLOW);
    lcd.setTextSize(2);
    lcd.setCursor(0, 0);
    lcd.print("#");
    lcd.setTextSize(3);
    lcd.print(String(retaliationCount));
    lcd.setCursor(115, 0);
    lcd.print(String(retaliationTime)); 

    lcd.setTextColor(PINK);
    lcd.setTextSize(2);
    lcd.setCursor(260, 0);
    lcd.println(bootDate);
    lcd.setTextSize(1);
    lcd.setCursor(290, 17);
    lcd.println(bootTime);

    LCDUpdateZone("A");
    LCDUpdateZone("B");
    LCDUpdateZone("C");
};

void LCDUpdateZone(String zone) {
    int xStart = -1;

    if (zone == "A") {
        xStart = 0;
    } else if (zone == "B") {
        xStart = 110;
    } else if (zone == "C") {
        xStart = 218;
    } else {
        Serial.println("Invalid zone specified!");
        return;
    }

    // Print Shock Label
    lcd.setTextColor(WHITE);
    lcd.setTextSize(2);
    lcd.setCursor(xStart, 109); // Y position for the shock label
    lcd.println(" SHOCK-" + zone);

    // Print Armed/Disarmed Status with adjusted text
      // Set text color and cursor position
      lcd.setTextColor(isArmed == "ARMED" ? RED : WHITE); // RED for armed, WHITE for disarmed
      lcd.setCursor(xStart, 125);                        // Y position for armed status

      // Print mixed font sizes
      lcd.setTextSize(1);
      lcd.print(isArmed == "DISARMED" ? " " : "    ");     // Smaller font space adjustment
      lcd.setTextSize(2);
      lcd.println(isArmed);                               // Larger font for status text


    // Print Vibration Magnitude and Threshold (VM/VT)
    lcd.setCursor(xStart, 141); // Y position for VM/VT
    lcd.setTextColor(ORANGE);
    lcd.print(String(vibrationMagnitude, 2));

    // Intentional spacing
    lcd.setTextSize(1);
    lcd.print(" ");
    lcd.setTextSize(2);

    lcd.setTextColor(LIGHT_BLUE);
    lcd.println(String(vibrationThreshold, 2));

    // Print Temperatures in Celsius and Fahrenheit
    lcd.setCursor(xStart, 157); // Y position for temperatures
    lcd.setTextColor(YELLOW);
    lcd.println(" " + String(temperatureC) + "C " + String(temperatureF) + "F");
}

// Required to store what color to use for older UpdateLog entries
struct LogColorDB {
    String text;
    uint16_t color;
};

void LCDUpdateLog() {
    const int maxLogRows = 4;

    // Determine the color for the current event
    uint16_t eventColor;
    if (event == "ARMED" || event == "DISARMED" || event == "REQUESTED" || event == "CONFIRMED" || event == "DETECTED") {
        eventColor = (isArmed == "ARMED" ? RED : WHITE);
    } else if (event == "CONNECTED") {
        eventColor = PINK;
    } else if (event == "ADJUSTED") {
        eventColor = GREEN; // Green for ADJUSTED
    } else if (event == "INCREASED") {
        eventColor = LIGHT_BLUE; // Light Blue for INCREASED
    } else if (event == "DECREASED") {
        eventColor = DEEP_PURPLE; // Deep Purple for DECREASED
    } else {
        eventColor = WHITE; // Default color
    }

    // Add new log entry
    if (logCount < MAX_LOGS) {
        for (int i = logCount; i > 0; i--) {
            logTextBuffer[i] = logTextBuffer[i - 1];
            logColorBuffer[i] = logColorBuffer[i - 1];
        }
        logCount++;
    } else {
        for (int i = MAX_LOGS - 1; i > 0; i--) {
            logTextBuffer[i] = logTextBuffer[i - 1];
            logColorBuffer[i] = logColorBuffer[i - 1];
        }
    }
    logTextBuffer[0] = dateDate + " " + dateTime + " " + ID + " " + event;
    logColorBuffer[0] = eventColor;

    // Clear and display logs
    LCDClearLog();
    lcd.setTextSize(2);
    int lineHeight = 20; // Adjusted for font size 2
    for (int i = 0; i < maxLogRows && i < logCount; i++) {
        lcd.setTextColor(logColorBuffer[i]);
        lcd.setCursor(0, 26 + (i * lineHeight)); // Adjust Y position for each log entry
        lcd.println(logTextBuffer[i]);
    }
}

// add anotther physical button, copy ther code where publishAdjustVibrationThreshold(-vtStep, true); exists, 
   // replace it with publishAdjustVibrationThreshold(-vtStep, true) or publishAdjustVibrationThreshold(vtStep, true);. and this should work flawlessly.
      // boolean value takes care of # or explicit
// vt step is currently set to 0.01 that is very small.. we may want to increase it..
//need to add another method to reset vt threshold? maybe.. we have step down so many not.. 