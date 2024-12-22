#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <time.h>
#include <HTTPClient.h>

// Task handle and queue
TaskHandle_t googleSheetsTaskHandle = NULL;
QueueHandle_t googleSheetsQueue;

// Define the maximum payload size for Google Sheets
#define MAX_PAYLOAD_SIZE 256


//Vibration Sensor Pin
#define VIBRATION_SENSOR_PIN 0  // Digital pin connected to SW-420 (change as needed)
#define DEBOUNCE_DELAY 100        // Debounce time in milliseconds
//Vibration Motor
#define VIBRATION_MOTOR_PIN 4

bool isArmed = false;
bool sensorEnabled = true; // Flag to control sensor detection

unsigned long lastVibrationTime = 0; // Tracks the last vibration event
unsigned long motorToggleTime = 0;  // Tracks motor on/off toggle time
bool motorActive = false;           // Indicates whether motor is active
unsigned long vibrationDelay = 4000; // Delay between detections when armed
unsigned long motorOnTime = 200;    // Motor ON duration in milliseconds
unsigned long motorOffTime = 200;   // Motor OFF duration in milliseconds


       
// Wi-Fi credentials (multiple SSIDs)
const char* wifi_credentials[][2] = {
    {"OP9", "aaaaaaaaa1"},
    {"icup +1", "aaaaaaaaa1"},
    {"ICU", "Emmajin6!"}
};

const int num_wifi_credentials = sizeof(wifi_credentials) / sizeof(wifi_credentials[0]);

// MQTT broker credentials
const char* mqtt_server = "9321cdfa0af34b83b77797a4488354cd.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "MasterA";
const char* mqtt_password = "MasterA1";
const char* mqtt_topic_CENTRAL_HUB = "CENTRAL-HUB";
const char* mqtt_topic_NTP = "NTP";
const char* mqtt_topic_WORKOUT_TIMER = "WORKOUT-TIMER";
const char* mqtt_topic_SHOCK_CENTER = "SHOCK-CENTER";
const char* clientID = "RESV-SHOCKERA";

// Your Google Apps Script Web App URL
const char* googleSheetURL = "https://script.google.com/macros/s/AKfycbzKXvDaj58CzF-1lKAhrHYrmOnBWA6omiKfFka9gppet9IrAt5zJSLdjh_7r1e9YjU-eQ/exec";


String requestedClientID = "";
String requestedTime = "";
String requestedStatus = "";



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

// WiFi and MQTT clients
WiFiClientSecure espClient;
PubSubClient client(espClient);


// NTP configuration
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC, sync every 60 seconds
unsigned long lastNTPFetch = 0;
#define SECONDS_IN_A_DAY 86400
#define MAX_NTP_RETRIES  6307200
int NTPReadyToPublish = 0;

// Internal time tracking
time_t internalTime = 0; // Tracks the current epoch time
unsigned long lastMillis = 0; // Tracks the last time the display was updated

// Connect to Wi-Fi
void setup_wifi() {
    Serial.println("Connecting to Wi-Fi...");
    int currentWiFiIndex = 0;

    while (WiFi.status() != WL_CONNECTED) {
        const char* ssid = wifi_credentials[currentWiFiIndex][0];
        const char* password = wifi_credentials[currentWiFiIndex][1];

        Serial.println("Trying SSID: " + String(ssid));

        WiFi.disconnect();  // Ensure any ongoing connection is terminated
        delay(100);         // Allow some time for the disconnect to take effect
        WiFi.begin(ssid, password);

        // Wait for connection or timeout (5 seconds)
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWIFI CONNECTED! SSID: \"" + String(ssid) + "\", IP: " + WiFi.localIP().toString());
            break; // Exit the loop once connected
        } else {
            Serial.println("\nFailed to connect to SSID: \"" + String(ssid) + "\". Trying next...");
            currentWiFiIndex = (currentWiFiIndex + 1) % num_wifi_credentials; // Loop through SSIDs
        }
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("ERROR: Unable to connect to any Wi-Fi network!");
        // Optionally handle fallback scenarios (e.g., start AP mode)
    }
}




// Fetch NTP time and update internal time
bool fetchAndSetNTPTime() {
  int retryCount = 0;
  while (!timeClient.update() && retryCount < MAX_NTP_RETRIES) {
    retryCount++;
    Serial.println("Retrying NTP fetch...");
    delay(2000);
  }

  if (retryCount == MAX_NTP_RETRIES) {
    Serial.println("Failed to fetch NTP time after retries.");
    return false;
  }

  internalTime = timeClient.getEpochTime();
  Serial.println("\nNTP Time fetched: " + String(ctime(&internalTime)));

  return true;
}

              // Publish timezone data to MQTT
              void publishTimeData() {
                const int offsets[] = {0, -5, -6, -7, -8};
                const char* zones[] = {"UTC-0", "EST-5", "CST-6", "MST-7", "PST-8"};
                char dateBuffer[20];
                char time12Buffer[10];
                char time24Buffer[10];

                String payload = "";

                for (int i = 0; i < 5; i++) {
                  time_t adjustedTime = internalTime + (offsets[i] * 3600);
                  struct tm* timeInfo = gmtime(&adjustedTime);

                  strftime(dateBuffer, sizeof(dateBuffer), "%m-%d", timeInfo);
                  strftime(time12Buffer, sizeof(time12Buffer), "%I:%M %p", timeInfo);
                  strftime(time24Buffer, sizeof(time24Buffer), "%H:%M", timeInfo);

                  payload += String(zones[i]) + " | " + String(dateBuffer) + " | " +
                            String(time12Buffer) + " | " + String(time24Buffer) + "\n";
                }

                Serial.println("\n----------- NTP TIME -----------\n" + payload);
                
                // Publish only EST-5 row separately
                int estIndex = 1; // Index of EST-5 in the offsets array
                time_t estTime = internalTime + (offsets[estIndex] * 3600);
                struct tm* estTimeInfo = gmtime(&estTime);

                strftime(dateBuffer, sizeof(dateBuffer), "%m/%d", estTimeInfo);
                strftime(time12Buffer, sizeof(time12Buffer), "%I:%M %p", estTimeInfo);
                strftime(time24Buffer, sizeof(time24Buffer), "%H:%M:%S", estTimeInfo);

                String estPayload = String(dateBuffer) + " " + String(time24Buffer);

                if (NTPReadyToPublish == 1){
                  //client.publish(mqtt_topic_NTP, estPayload.c_str());
                  client.publish(mqtt_topic_CENTRAL_HUB, (String(clientID) + " | CONNECTED | " + estPayload).c_str());
                } else {
                  Serial.println("Staged NTP could not publish, NTPReadyToPublish flag remains at 0\n");
                }
              }

void callback(char* topic, byte* payload, unsigned int length) {
    // Convert payload to a string
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    message.trim(); // Remove leading/trailing whitespace

    // Debug: Print raw message and topic
    Serial.println("RESV-SHOCKERA RCVD [" + String(topic) + "]: " + message);

    // Ensure we are processing CENTRAL-HUB messages
    if (String(topic) != mqtt_topic_CENTRAL_HUB) {
        Serial.println("Ignored: Not from CENTRAL-HUB.");
        return;
    }

    // Check if the message contains "CONNECTED"
    if (message.indexOf("CONNECTED") != -1) {
        // Parse message content
        int pos1 = message.indexOf(':');
        int pos2 = message.indexOf('|');
        int pos3 = message.lastIndexOf('|');

        // Validate positions of the delimiters
        if (pos1 == -1 || pos2 == -1 || pos3 == -1) {
            Serial.println("Error: Invalid CONNECTED message format");
            return;
        }

        // Extract Connected ClientID
        String connectedClientID = message.substring(0, pos2); // Adjust for space after ':'
        connectedClientID.trim();

        // Extract Date and Time (after the last delimiter)
        String dateTime = message.substring(pos3 + 1);
        dateTime.trim();

        // Create payload for Google Sheets
        String payload = "{\n";
        payload += "  \"clientID\": \"" + String(clientID) + "\",\n"; // Current ESP
        payload += "  \"status\": \"" + connectedClientID + " CONNECTED\",\n"; // Connected client
        payload += "  \"armed\": \"--\",\n";
        payload += "  \"dateTime\": \"" + dateTime + "\"\n";
        payload += "}";

        if (payload.length() >= MAX_PAYLOAD_SIZE) {
            Serial.println("ERROR: Payload exceeds maximum size");
            return;
        }

        // Enqueue the payload for the RTOS task
        if (xQueueSend(googleSheetsQueue, payload.c_str(), portMAX_DELAY) != pdPASS) {
            Serial.println("ERROR: Failed to enqueue CONNECTED payload");
        } else {
            Serial.println("INFO: CONNECTED payload enqueued");
        }
        return;
    }

    // Validate the message contains "REQUESTED ARM" or "REQUESTED DISARM"
    if (message.indexOf("REQUESTED ARM") == -1 && message.indexOf("REQUESTED DISARM") == -1) {
        Serial.println("Ignored: No ARM/DISARM request found.");
        return;
    }

    // Parse message content for ARM/DISARM
    int pos1 = message.indexOf('|');
    int pos2 = message.indexOf('|', pos1 + 1);

    // Validate positions of the delimiters
    if (pos1 == -1 || pos2 == -1) {
        Serial.println("Error: Invalid message format");
        return;
    }

    // Extract ClientID
    requestedClientID = message.substring(0, pos1);
    requestedClientID.trim();

    // Extract Status (REQUESTED ARM or REQUESTED DISARM)
    requestedStatus = message.substring(pos1 + 1, pos2);
    requestedStatus.trim();

    // Extract Date and Time (after the second delimiter)
    requestedTime = message.substring(pos2 + 1);
    requestedTime.trim();

    // Update isArmed based on the requested status
    if (requestedStatus == "REQUESTED ARM") {
        isArmed = true;
    } else if (requestedStatus == "REQUESTED DISARM") {
        isArmed = false;

        // Disable motor immediately
        motorActive = false;
        digitalWrite(VIBRATION_MOTOR_PIN, LOW);
    } else {
        Serial.println("Error: MQTT value for STATUS is incorrect");
        return; // Exit if status is invalid
    }

    // Send data to Google Sheets
    sendRequestsToSheets(requestedClientID, requestedStatus, isArmed ? "ARMED" : "DISARMED", requestedTime);

    // Send a confirmation response
    respondToCentralHub();
}



              // Reconnect to MQTT broker
              void reconnect() {
                while (!client.connected()) {
                  Serial.print("Connecting to MQTT broker...");
                  if (client.connect(clientID, mqtt_user, mqtt_password)) {
                    Serial.print("CONNECTED!\n\n");

                    client.subscribe(mqtt_topic_CENTRAL_HUB);
                   // client.publish(mqtt_topic_CENTRAL_HUB, (String(clientID) + " CONNECTED").c_str());

                    NTPReadyToPublish = 1;

                    Serial.println(String(clientID) + " | FULLY SUBSCRIBED TO [" + mqtt_topic_CENTRAL_HUB + "] | [ ONLY PUBLISHING TO [" + mqtt_topic_SHOCK_CENTER + " ]");

                    publishTimeData();

                  } else {
                    Serial.println(" | MQTT BROKER CONNECTION FAILED!, rc=" + String(client.state()));
                    delay(5000);
                  }
                }
              }

void setup() {
    Serial.begin(921600);
    setup_wifi();

    // Initialize vibration sensor and motor pins
    pinMode(VIBRATION_SENSOR_PIN, INPUT_PULLDOWN);
    pinMode(VIBRATION_MOTOR_PIN, OUTPUT);
    digitalWrite(VIBRATION_MOTOR_PIN, LOW); // Ensure motor is off initially

    // Initialize variables
    lastVibrationTime = millis();
    motorActive = false;

    // Configure MQTT client
    espClient.setCACert(root_ca);
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    // Start NTP client
    timeClient.begin();

    // Fetch and set NTP time
    if (fetchAndSetNTPTime()) {
        lastNTPFetch = millis();
    } else {
        Serial.println("WARNING NTP COULD NOT CONNECT, USING DEFAULT TIME 11/11/11 11:11 UTC");
        internalTime = 1321019471; // 11/11/11 11:11 UTC
    }

    // Create queue for Google Sheets communication
    googleSheetsQueue = xQueueCreate(10, MAX_PAYLOAD_SIZE);
    if (googleSheetsQueue == NULL) {
        Serial.println("ERROR: Failed to create queue for Google Sheets");
    }

    // Create RTOS task for Google Sheets
    xTaskCreate(
        googleSheetsTask,           // Task function
        "GoogleSheetsTask",         // Task name
        4096,                       // Stack size
        NULL,                       // Task parameter
        1,                          // Task priority
        &googleSheetsTaskHandle     // Task handle
    );

    if (googleSheetsTaskHandle == NULL) {
        Serial.println("ERROR: Failed to create Google Sheets task");
    }
}

void loop() {
    static unsigned long lastWiFiCheck = 0;  // Tracks the last time Wi-Fi status was checked
    const unsigned long wifiCheckInterval = 5000;  // Check Wi-Fi every 5 seconds
    static int currentWiFiIndex = 0;  // Tracks which Wi-Fi SSID to try next
    static bool wifiDisconnected = false;  // Tracks if Wi-Fi is currently disconnected

    unsigned long currentMillis = millis();

    // Periodic Wi-Fi status check
    if (currentMillis - lastWiFiCheck >= wifiCheckInterval) {
        lastWiFiCheck = currentMillis;

        if (WiFi.status() != WL_CONNECTED) {
            if (!wifiDisconnected) {
                Serial.println("Wi-Fi disconnected! Attempting to reconnect...");
                wifiDisconnected = true;  // Set the disconnected flag
            }

            // Disconnect and move to the next Wi-Fi SSID
            WiFi.disconnect();
            delay(100);  // Allow disconnect to take effect
            currentWiFiIndex = (currentWiFiIndex + 1) % num_wifi_credentials;  // Loop through SSIDs

            const char* ssid = wifi_credentials[currentWiFiIndex][0];
            const char* password = wifi_credentials[currentWiFiIndex][1];

            Serial.println("Trying SSID: " + String(ssid));
            WiFi.begin(ssid, password);

            // Wait for connection or timeout (5 seconds)
            unsigned long startAttemptTime = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
                delay(500);
                Serial.print(".");
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\nWi-Fi reconnected! SSID: \"" + String(ssid) + "\", IP: " + WiFi.localIP().toString());
                wifiDisconnected = false;  // Reset the disconnected flag
            } else {
                Serial.println("\nFailed to reconnect to SSID: \"" + String(ssid) + "\". Will try next...");
            }
        }
    }

    // Ensure Wi-Fi is connected before attempting MQTT reconnection
    if (WiFi.status() == WL_CONNECTED) {
        // Ensure MQTT client is connected
        if (!client.connected()) {
            reconnect();
        }
        client.loop();  // Process MQTT messages
    } else if (!wifiDisconnected) {
        Serial.println("Skipping MQTT reconnect as Wi-Fi is not connected.");
        wifiDisconnected = true;  // Set the disconnected flag to avoid repeated messages
    }

    // Increment `internalTime` every 60 seconds
    if (currentMillis - lastMillis >= 60000) {
        internalTime += 60;  // Increment by 60 seconds
        lastMillis = currentMillis;
    }

    // Update NTP time once a day
    if (currentMillis - lastNTPFetch >= SECONDS_IN_A_DAY * 1000) {
        if (fetchAndSetNTPTime()) {
            lastNTPFetch = currentMillis;
        }
    }

    // Vibration handling logic
    handleVibration();  // No change; now it just pushes to the queue
}



void respondToCentralHub() {
    if (requestedClientID.isEmpty() || requestedStatus.isEmpty() || requestedTime.isEmpty()) {
        Serial.println("Debug: Missing data for response. Skipping publish.");
        return;
    }

    String action = isArmed ? "ARM" : "DISARM";

    // Prepare the confirmation message for MQTT
    String confirmationMessage = requestedClientID + " | " + action + " CONFIRMED | " + requestedTime;

    // Publish the confirmation to the CENTRAL-HUB topic
    if (client.connected()) {
        client.publish(mqtt_topic_CENTRAL_HUB, confirmationMessage.c_str());

        // Correctly format status for Google Sheets
        String status = requestedClientID + " " + action + " CONFIRMED";
        String armed = isArmed ? "ARMED" : "DISARMED";

        // Send confirmation to Google Sheets
        sendConfirmationsToSheets(clientID, status, armed, requestedTime);
    } else {
        Serial.println("Error: MQTT client not connected. Failed to publish confirmation.");
    }
}

void handleVibration() {
    unsigned long currentMillis = millis();

    // Always use the same delay (vibrationDelay) for subsequent shocks
    unsigned long detectionDelay = vibrationDelay;

    // Check for vibration detection
    if (digitalRead(VIBRATION_SENSOR_PIN) == HIGH) {
        if (currentMillis - lastVibrationTime >= detectionDelay) {
            // Log shock event
            time_t currentTime = internalTime + ((currentMillis - lastMillis) / 1000);
            time_t estTime = currentTime - (5 * 3600); // Adjust for EST
            struct tm* timeInfo = gmtime(&estTime);

            char dateTimeBuffer[20];
            snprintf(dateTimeBuffer, sizeof(dateTimeBuffer), "%02d/%02d %02d:%02d:%02d", 
                     timeInfo->tm_mon + 1, timeInfo->tm_mday,
                     timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);

            String message = String(clientID) + " | DETECTED SHOCK | " +
                             (isArmed ? "ARMED" : "DISARMED") + " | " + dateTimeBuffer;

            // Print to Serial
            Serial.println(message);
           
            // Publish to MQTT
            client.publish(mqtt_topic_SHOCK_CENTER, message.c_str());

            

            lastVibrationTime = currentMillis;

            // Activate motor if armed
            if (isArmed) {
                motorActive = true;
                motorToggleTime = currentMillis;
                digitalWrite(VIBRATION_MOTOR_PIN, HIGH);
            }
            // Send data to Google Sheets
            sendDetectionsToSheets(clientID, "DETECTED SHOCK", isArmed ? "ARMED" : "DISARMED", dateTimeBuffer);
        }
    }

    // Handle motor toggle if active
    if (motorActive) {
        if ((currentMillis - motorToggleTime >= motorOnTime) && digitalRead(VIBRATION_MOTOR_PIN) == HIGH) {
            digitalWrite(VIBRATION_MOTOR_PIN, LOW); // Turn motor off
            motorToggleTime = currentMillis;       // Reset toggle timer
        } else if ((currentMillis - motorToggleTime >= motorOffTime) && digitalRead(VIBRATION_MOTOR_PIN) == LOW) {
            digitalWrite(VIBRATION_MOTOR_PIN, HIGH); // Turn motor on
            motorToggleTime = currentMillis;        // Reset toggle timer
        }

        // Stop motor after vibration delay
        if (currentMillis - lastVibrationTime >= vibrationDelay) {
            motorActive = false;
            digitalWrite(VIBRATION_MOTOR_PIN, LOW); // Ensure motor is off
        }
    }
}

// Function to enqueue data for Google Sheets
void sendDetectionsToSheets(String clientID, String status, String armed, String dateTimeBuffer) {
    if (WiFi.status() == WL_CONNECTED) {
        // Create JSON payload
        String payload = "{\"clientID\":\"" + clientID + "\",\"status\":\"" + status +
                         "\",\"armed\":\"" + armed + "\",\"dateTime\":\"" + dateTimeBuffer + "\"}";

        if (payload.length() >= MAX_PAYLOAD_SIZE) {
            Serial.println("ERROR: Payload exceeds maximum size");
            return;
        }

        // Enqueue the payload for the RTOS task
        if (xQueueSend(googleSheetsQueue, payload.c_str(), portMAX_DELAY) != pdPASS) {
            Serial.println("ERROR: Failed to enqueue payload");
        }
    } else {
        Serial.println("WiFi Disconnected");
    }
}

void sendRequestsToSheets(String requestedClientID, String status, String armed, String fullTimestamp) {
    // Create JSON payload
    String payload = "{\n";
    payload += "  \"clientID\": \"" + String(clientID) + "\",\n";
    payload += "  \"status\": \"" + requestedClientID + " " + status + "\",\n";
    payload += "  \"armed\": \"" + armed + "\",\n";
    payload += "  \"dateTime\": \"" + fullTimestamp + "\"\n";
    payload += "}";

    if (payload.length() >= MAX_PAYLOAD_SIZE) {
        Serial.println("ERROR: Payload exceeds maximum size");
        return;
    }

    // Enqueue the payload
    if (xQueueSend(googleSheetsQueue, payload.c_str(), portMAX_DELAY) != pdPASS) {
        Serial.println("ERROR: Failed to enqueue request payload");
    } else {
        Serial.println("INFO: Request payload enqueued");
    }
}


void sendConfirmationsToSheets(String ClientID, String status, String armed, String fullTimestamp) {
    // Create JSON payload
    String payload = "{\n";
    payload += "  \"clientID\": \"" + String(ClientID) + "\",\n";
    payload += "  \"status\": \"" + status + "\",\n";
    payload += "  \"armed\": \"" + armed + "\",\n";
    payload += "  \"dateTime\": \"" + fullTimestamp + "\"\n";
    payload += "}";

    if (payload.length() >= MAX_PAYLOAD_SIZE) {
        Serial.println("ERROR: Payload exceeds maximum size");
        return;
    }

    // Enqueue the payload
    if (xQueueSend(googleSheetsQueue, payload.c_str(), portMAX_DELAY) != pdPASS) {
        Serial.println("ERROR: Failed to enqueue confirmation payload");
    } else {
        Serial.println("INFO: Confirmation payload enqueued");
    }
}



// Task for Google Sheets communication
void googleSheetsTask(void* parameter) {
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
                    Serial.println("ERROR SENDING PAYLOAD TO GOOGLE SHEETS");
                }

                http.end();
            } else {
                Serial.println("ERROR: Wi-Fi not connected. Unable to send to Google Sheets.");
            }
        }
    }
}

