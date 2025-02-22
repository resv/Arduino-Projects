#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <time.h>

// LCD configuration
#define LCD_WIDTH 170
#define LCD_HEIGHT 320
#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_CS 15
#define LCD_DC 2
#define LCD_RST 4
#define LCD_BLK 32

#define EXTERNAL_BUTTON_PIN 22  // GPIO pin for the external button
#define ONBOARD_BUTTON_PIN 0   // GPIO pin for the onboard button


Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

// Wi-Fi credentials (multiple SSIDs)
const char* wifi_credentials[][2] = {
    {"OP9", "aaaaaaaaa1"},
    {"icup +1", "aaaaaaaaa1"},
    {"ICU", "Emmajin6!"}
};
const int num_wifi_credentials = sizeof(wifi_credentials) / sizeof(wifi_credentials[0]);

// Other configurations and variables
unsigned long lastWiFiCheck = 0;  // Track last Wi-Fi check time
const unsigned long wifiCheckInterval = 5000;  // Interval for Wi-Fi check (5 seconds)
int currentWiFiIndex = 0;  // Index for cycling through SSIDs
bool mqttReconnectBlocked = false;  // Block MQTT reconnect spam

// MQTT broker credentials
const char* mqtt_server = "9321cdfa0af34b83b77797a4488354cd.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "MasterA";
const char* mqtt_password = "MasterA1";
const char* mqtt_topic_CENTRAL_HUB = "CENTRAL-HUB";
const char* mqtt_topic_NTP = "NTP";
const char* mqtt_topic_WORKOUT_TIMER = "WORKOUT-TIMER";
const char* mqtt_topic_SHOCK_CENTER = "SHOCK-CENTER";
const char* clientID = "RESV-1ST";

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

// Default workout timer value
String workoutTimer = "--:--";

// Default date and time since boot
String shockTimeSinceBoot = " --:--";
String dateSinceBoot = "--/--";

// Status line variables
String statusDetection = "Listening";  //
String isArmed = "";              // STATUS: ---
String lastRequestClientID = "";    // LAST REQUEST: --- (CLIENTID)
String lastRequestStatus = "";      // LAST REQUEST: --- ARM/DISARM REQUESTED
String lastRequestDate = "";        // LAST REQUEST: --- (DATE)
String lastRequestTime = "";        // LAST REQUEST: --- (DATE / TIME)

// Define the log size and initialize the log
const int logSize = 50;
String lastDetectionLog[logSize];

int totalRetaliationCount = 0;  // TOTAL RETALIATION START TIME

// NTP configuration
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);  // UTC, sync every 60 seconds
unsigned long lastNTPFetch = 0;
#define SECONDS_IN_A_DAY 86400
#define MAX_NTP_RETRIES 6307200
int NTPReadyToPublish = 0;

// Internal time tracking
time_t internalTime = 0;       // Tracks the current epoch time
unsigned long lastMillis = 0;  // Tracks the last time the display was updated

// Connect to Wi-Fi
void setup_wifi() {
    Serial.println("Connecting to Wi-Fi...");

    for (int i = 0; i < num_wifi_credentials; i++) {
        const char* ssid = wifi_credentials[i][0];
        const char* password = wifi_credentials[i][1];

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
            Serial.println("\nWi-Fi connected! SSID: \"" + String(ssid) + "\", IP: " + WiFi.localIP().toString());
            return; // Exit once connected
        } else {
            Serial.println("\nFailed to connect to SSID: \"" + String(ssid) + "\". Trying next...");
        }
    }

    Serial.println("ERROR: Unable to connect to any Wi-Fi network!");
    // Optionally: Trigger a fallback behavior here (e.g., retry loop, enter AP mode, etc.)
}

// Function to update the timer on the LCD
void updateWorkoutTimerLCD(const String& timerValue) {
  lcd.fillRect(199, 0, 70, 14, ST77XX_BLACK);  // Clear timer area
  lcd.setCursor(199, 0);                       // Set cursor position for timer
  lcd.setTextSize(2);                          // Set text size for timer
  lcd.setTextColor(ST77XX_GREEN);
  lcd.println(timerValue);  // Display the timer
}


// Fetch NTP time and update internal time
bool fetchAndSetNTPTime() {
  int retryCount = 0;

  lcd.fillRect(0, 100, 320, 70, ST77XX_BLACK);
  lcd.setCursor(30, 120);
  lcd.setTextSize(3);
  lcd.setTextColor(ST77XX_WHITE);
  lcd.println("Fetching NTP...");

  while (!timeClient.update() && retryCount < MAX_NTP_RETRIES) {
    retryCount++;
    Serial.println("Retrying NTP fetch...");
    delay(2000);
  }

  if (retryCount == MAX_NTP_RETRIES) {
    Serial.println("Failed to fetch NTP time after retries.");

    lcd.fillRect(0, 100, 320, 70, ST77XX_BLACK);
    lcd.setCursor(40, 120);
    lcd.setTextSize(3);
    lcd.setTextColor(ST77XX_YELLOW);
    lcd.println("NTP FAILED");
    return false;
  }

  internalTime = timeClient.getEpochTime();
  Serial.println("NTP Time fetched: " + String(ctime(&internalTime)));

  // Publish updated times after fetch
  publishTimeData();

  return true;
}

// Display and format all timezone data
void displayTimezones() {
  const int offsets[] = { 0, -5, -6, -7, -8 };
  const char* zones[] = { "UTC-0", "EST-5", "CST-6", "MST-7", "PST-8" };
  const uint16_t zoneColors[] = { ST77XX_CYAN, ST77XX_GREEN, ST77XX_WHITE, ST77XX_ORANGE, ST77XX_MAGENTA };
  char dateBuffer[20];
  char time12Buffer[10];
  char time24Buffer[10];

  const int xOffset = 3;  // Offset to shift all x-coordinates

  // Display table headers
  lcd.setTextSize(2);
  lcd.fillRect(0, 100, 320, 70, ST77XX_BLACK);

  // Sketch lines for table for timezones:
  lcd.fillRect(0, 100, 320, 1, ST77XX_YELLOW);   // horizontal line
  lcd.fillRect(64, 100, 1, 70, ST77XX_YELLOW);   // vertical line
  lcd.fillRect(128, 100, 1, 70, ST77XX_YELLOW);  // vertical line
  lcd.fillRect(192, 100, 1, 70, ST77XX_YELLOW);  // vertical line
  lcd.fillRect(256, 100, 1, 70, ST77XX_YELLOW);  // vertical line

  // Sketch fill in colors for table for timezones:
  //lcd.fillRect(0, 100, 64, 70, ST77XX_BLUE);      // PST CELL
  //lcd.fillRect(64, 100, 128, 70, ST77XX_RED);     // MST CELL
  //lcd.fillRect(128, 100, 192, 70, ST77XX_ORANGE); // CST CELL
  //lcd.fillRect(192, 100, 256, 70, ST77XX_GREEN);  // EST CELL
  //lcd.fillRect(256, 100, 320, 70, ST77XX_WHITE);  // UTC CELL

  // Loop through each timezone and display zone names and times
  for (int i = 4; i >= 0; i--) {
    // Set text color for the current zone
    lcd.setTextColor(zoneColors[i]);

    // Print zone name
    lcd.setCursor((64 * (4 - i)) + xOffset, 102);  // Add xOffset to x-coordinate
    lcd.print(zones[i]);

    //lcd.setTextColor(ST77XX_WHITE); //back to white for the rest
    // Adjust time for the current timezone
    time_t adjustedTime = internalTime + (offsets[i] * 3600);
    struct tm* timeInfo = gmtime(&adjustedTime);

    strftime(dateBuffer, sizeof(dateBuffer), "%m/%d", timeInfo);
    strftime(time12Buffer, sizeof(time12Buffer), "%I:%M", timeInfo);  // 12-hour format without AM/PM
    strftime(time24Buffer, sizeof(time24Buffer), "%H:%M", timeInfo);

    // Print time in 12-hour format (without AM/PM)
    lcd.setCursor((64 * (4 - i)) + xOffset, 120);
    lcd.print(time12Buffer);

    // Print time in 24-hour format
    lcd.setCursor((64 * (4 - i)) + xOffset, 138);
    lcd.print(time24Buffer);

    // Print date
    lcd.setCursor((64 * (4 - i)) + xOffset, 156);
    lcd.print(dateBuffer);
    
    // Update `dateSinceBoot` if processing EST
    if (i == 1) { // Index 1 corresponds to EST
        dateSinceBoot = String(dateBuffer); // Save the date in global variable
    }
  }
}

// Publish timezone data to MQTT
void publishTimeData() {
  const int offsets[] = { 0, -5, -6, -7, -8 };
  const char* zones[] = { "UTC-0", "EST-5", "CST-6", "MST-7", "PST-8" };
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

    payload += String(zones[i]) + " | " + String(dateBuffer) + " | " + String(time12Buffer) + " | " + String(time24Buffer) + "\n";
  }

  //client.publish(mqtt_topic_NTP, payload.c_str()); (DOESN"T WORK DUE TO HIVEMQ MQTT CHAR LIMIT?)
  Serial.println("----------- NTP TIME -----------\n" + payload);

  // Publish only EST-5 row separately
  int estIndex = 1;  // Index of EST-5 in the offsets array
  time_t estTime = internalTime + (offsets[estIndex] * 3600);
  struct tm* estTimeInfo = gmtime(&estTime);

  strftime(dateBuffer, sizeof(dateBuffer), "%m/%d", estTimeInfo);
  strftime(time12Buffer, sizeof(time12Buffer), "%I:%M %p", estTimeInfo);
  strftime(time24Buffer, sizeof(time24Buffer), "%H:%M:%S", estTimeInfo);

  String estPayload = String(dateBuffer) + " " + String(time24Buffer);

  Serial.println("Staged NTP Publish data:");
  Serial.println(estPayload + "\n");

  if (NTPReadyToPublish == 1) {
    //client.publish(mqtt_topic_NTP, estPayload.c_str());
    client.publish(mqtt_topic_CENTRAL_HUB, (String(clientID) + " | CONNECTED | " + estPayload).c_str());
  } else {
    Serial.println("Staged NTP could not publish, NTPReadyToPublish flag remains at 0\n");
  }
}

// Update callback to handle WORKOUT-TIMER messages and CENTRAL-HUB logs
// Updated callback function
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println("RESV-1ST RCVD [" + String(topic) + "]: " + message);

  // Handle NTP requests
  if (String(topic) == mqtt_topic_NTP && message.endsWith("| NTP REQUEST")) {
    Serial.println("Processing NTP REQUEST...");
    if (fetchAndSetNTPTime()) {
      Serial.println("NTP updated and published.");
    }
  }

  // Handle WORKOUT-TIMER updates
  if (String(topic) == mqtt_topic_WORKOUT_TIMER) {
    workoutTimer = message;
    updateWorkoutTimerLCD(workoutTimer);
  }

  // Handle messages from SHOCK-CENTER with "DETECTED SHOCK"
  if (String(topic) == mqtt_topic_SHOCK_CENTER && message.indexOf("DETECTED SHOCK") != -1) {
    // Parse and format the message
    String formattedMessage = parseAndFormatMQTTMessage(message);

    // Add the formatted message to the log
    addToLog(formattedMessage);

    // Update the LCD with the latest logs
    updateLastDetectionLine(); //log column
    updateIsArmedLine();  //corner icon
    totalRetaliationCount++;
    updateTotalRetaliationLine(); // Banner retaliation count
    updateShockTimeSinceBoot();  // the top of the top, middle
  }
  
   if (String(topic) == mqtt_topic_CENTRAL_HUB) {
    if (message.indexOf("ARM CONFIRMED") != -1 || message.indexOf("DISARM CONFIRMED") != -1) {
      // Parse and format the message
      String formattedMessage = parseAndFormatCentralHubMessage(message);
      // Update the LCD
      updateLastRequest();
      updateIsArmedLine();
    }
  }

}

// Reconnect to MQTT broker
void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT broker...");
    if (client.connect(clientID, mqtt_user, mqtt_password)) {
      Serial.println(String(clientID) + " | CONNECTED TT MQTT BROKER!");

      client.subscribe(mqtt_topic_CENTRAL_HUB);

      client.subscribe(mqtt_topic_NTP);
      //client.publish(mqtt_topic_NTP, (String(clientID) + " CONNECTED").c_str());
      NTPReadyToPublish = 1;

      client.subscribe(mqtt_topic_WORKOUT_TIMER);
      //client.publish(mqtt_topic_WORKOUT_TIMER, (String(clientID) + " CONNECTED").c_str());

      client.subscribe(mqtt_topic_SHOCK_CENTER);

      Serial.println(String(clientID) + " | FULLY SUBSCRIBED TO [" + mqtt_topic_CENTRAL_HUB + "] | [" + mqtt_topic_NTP + "] | [" + mqtt_topic_WORKOUT_TIMER + "] | [" + mqtt_topic_SHOCK_CENTER + "]");

      publishTimeData();

    } else {
      Serial.println(" | MQTT BROKER CONNECTION FAILED!, rc=" + String(client.state()));
      delay(5000);
    }
  }
}

// Setup function
void setup() {
  Serial.begin(115200);
  setup_wifi();

  espClient.setCACert(root_ca);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  lcd.init(LCD_WIDTH, LCD_HEIGHT);
  lcd.setRotation(1);
  lcd.fillRect(0, 0, 320, 100, ST77XX_BLACK);

  pinMode(EXTERNAL_BUTTON_PIN, INPUT_PULLUP);  // External button as input with pull-up
  pinMode(ONBOARD_BUTTON_PIN, INPUT_PULLUP);   // Onboard button as input with pull-up


  //  lcd.setCursor(40, 40);
  //  lcd.setTextSize(3);
  //  lcd.setTextColor(ST77XX_WHITE);
  //  lcd.println("Fetching NTP...");

  timeClient.begin();

  if (fetchAndSetNTPTime()) {
    lastNTPFetch = millis();
  } else {
    Serial.println("Using default time.");
    internalTime = 0;  // 11/11/11 11:11 UTC
  }

  displayTimezones();
  updateWorkoutTimerLCD(workoutTimer);  // Display default timer value

  // Display Status Lines
  updateStatusLine(); // Banner Detection/listening
  updateIsArmedLine(); //corner icon
  updateLastRequest();
  updateLastDetectionLine(); //log column
  updateTotalRetaliationLine(); // Banner retaliation count
  updateDateSinceBoot(); // the Bottom of the top, middle
  updateShockTimeSinceBoot(); // the top of the top, middle
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
        if (!client.connected()) {
            reconnect();  // Reconnect to MQTT if disconnected
        }
        client.loop();  // Process MQTT messages
    } else if (!wifiDisconnected) {
        Serial.println("Skipping MQTT reconnect as Wi-Fi is not connected.");
        wifiDisconnected = true;  // Avoid repeated disconnect messages
    }

    // Update the displayed time every 60 seconds
    if (currentMillis - lastMillis >= 60000) {
        internalTime += 60;         // Increment internal time by 60 seconds
        displayTimezones();         // Update time zones on LCD
        lastMillis = currentMillis; // Reset last time display was updated
    }

    // Refresh NTP time once a day
    if (currentMillis - lastNTPFetch >= SECONDS_IN_A_DAY * 1000) {
        if (fetchAndSetNTPTime()) {
            lastNTPFetch = currentMillis;  // Reset NTP fetch timer
        }
    }

    // Check for button presses
    buttonPressToggle();
}



void updateStatusLine() {
  //lcd.fillRect(0, 0, 190, 14, ST77XX_BLACK);
  lcd.setCursor(0, 0);
  lcd.setTextSize(3);
  lcd.setTextColor(ST77XX_WHITE);
  lcd.print("");
  lcd.setTextColor(ST77XX_YELLOW);
  lcd.print(statusDetection);  // or DETECTED!
  lcd.setTextColor(ST77XX_WHITE);
  lcd.print("");
}

void updateLastDetectionLine() {
  // Clear the area on the LCD where the detection logs are displayed
  lcd.fillRect(0, 25, 192, 145, ST77XX_BLACK); // TOP LEFT CORNER TO SYMBOL AND DOWN CLEAR
  lcd.fillRect(192, 100, 128, 70, ST77XX_BLACK); // BOTTOM RIGHT CORNER OF UTC CLEAR
  
  // Display the most recent logs
  lcd.setTextSize(2);
  lcd.setTextColor(ST77XX_WHITE);

  // Loop through the first few logs (e.g., the most recent 7 logs)
  for (int i = 0; i < 7 && i < 50; i++) {
    if (lastDetectionLog[i] != "") {    // Check if the log exists
      lcd.setCursor(0, 25 + (i * 20));  // Adjust the Y-position for each log
      lcd.print(lastDetectionLog[i]);   // Display the log
    }
  }
}

void updateLastRequest() {
    lcd.fillRect(189, 42, 70, 17, ST77XX_BLACK); // LAST word clear
    lcd.fillRect(190, 59, 130, 40, ST77XX_BLACK); // Variables word clear
    if (isArmed == "ARMED"){
      lcd.setTextColor(ST77XX_ORANGE);
    } else if (isArmed == "DISAR"){
      lcd.setTextColor(ST77XX_CYAN);    
    } else {
      lcd.setTextColor(ST77XX_GREEN);  
    }
      lcd.setCursor(190, 43);
      lcd.setTextSize(2);
      lcd.print(" LAST");
      lcd.setCursor(195, 60);
      lcd.print(lastRequestClientID);
      lcd.setCursor(192, 77);
      lcd.print(lastRequestDate);
      lcd.setCursor(259, 77);
      lcd.print(lastRequestTime);
}

void updateTotalRetaliationLine() {
    // Lambda function for formatting numbers with commas
    auto formatNumberWithCommas = [](long number) -> String {
        String result = "";
        int count = 0;

        if (number == 0) {
            return "0";
        }

        while (number > 0) {
            result = (char)((number % 10) + '0') + result; // Add the digit
            count++;
            if (count % 3 == 0 && number / 10 > 0) {
                result = ',' + result; // Add comma every three digits
            }
            number /= 10;
        }

        return result;
    };

    // Format the count with commas
    String formattedCount = formatNumberWithCommas(totalRetaliationCount);

    if (totalRetaliationCount <= 9) {                             //0-9
        lcd.fillRect(0, -1, 187, 26, ST77XX_BLACK);
        lcd.setCursor(0, 0);
        lcd.setTextSize(3);
        lcd.setTextColor(ST77XX_YELLOW);
        lcd.print("    #");
        lcd.print(formattedCount);
        lcd.print("   ");
    } else if (totalRetaliationCount >= 10 && totalRetaliationCount <= 99) {    //10-99
        lcd.fillRect(0, -1, 187, 26, ST77XX_BLACK);
        lcd.setCursor(0, 0);
        lcd.setTextSize(3);
        lcd.setTextColor(ST77XX_YELLOW);
        lcd.print("   #");
        lcd.print(formattedCount);
        lcd.print("   ");
    } else if (totalRetaliationCount >= 100 && totalRetaliationCount <= 999) { //100-999
        lcd.fillRect(0, -1, 187, 26, ST77XX_BLACK);
        lcd.setCursor(0, 0);
        lcd.setTextSize(3);
        lcd.setTextColor(ST77XX_YELLOW);
        lcd.print("   #");
        lcd.print(formattedCount);
        lcd.print("  ");
    } else if (totalRetaliationCount >= 1000 && totalRetaliationCount <= 9999) { //1,000-9,999
        lcd.fillRect(0, -1, 187, 26, ST77XX_BLACK);
        lcd.setCursor(0, 0);
        lcd.setTextSize(3);
        lcd.setTextColor(ST77XX_YELLOW);
        lcd.print("  #");
        lcd.print(formattedCount);
        lcd.print("  ");
    } else if (totalRetaliationCount >= 10000 && totalRetaliationCount <= 99999) { //10,000-99,999
        lcd.fillRect(0, -1, 187, 26, ST77XX_BLACK);
        lcd.setCursor(0, 0);
        lcd.setTextSize(3);
        lcd.setTextColor(ST77XX_YELLOW);
        lcd.print("  #");
        lcd.print(formattedCount);
        lcd.print(" ");
    } else if (totalRetaliationCount >= 100000 && totalRetaliationCount <= 999999) { //100,000-999,999
        lcd.fillRect(0, -1, 187, 26, ST77XX_BLACK);
        lcd.setCursor(0, 0);
        lcd.setTextSize(3);
        lcd.setTextColor(ST77XX_YELLOW);
        lcd.print(" #");
        lcd.print(formattedCount);
        lcd.print(" ");
    } else if (totalRetaliationCount >= 1000000 && totalRetaliationCount <= 9999999) { //1,000,000-9,999,999
        lcd.fillRect(0, -1, 187, 26, ST77XX_BLACK);
        lcd.setCursor(0, 0);
        lcd.setTextSize(3);
        lcd.setTextColor(ST77XX_YELLOW);
        lcd.print("#");
        lcd.print(formattedCount);
        lcd.print("");
    }
}


void updateIsArmedLine() {
    static String lastIsArmed = ""; // Track the last state

    if (isArmed == lastIsArmed) {
        return; // Exit if the state hasn't changed
    }

    // Clear the area for the icon and text
    lcd.fillRect(260, 0, 60, 57, ST77XX_BLACK);

    // Common icon display settings
    lcd.setCursor(276, 0);
    lcd.setTextSize(6);

    if (isArmed == " N/A ") {
        lcd.setTextColor(ST77XX_WHITE);
        lcd.print("?");
    } else if (isArmed == "ARMED") {
        lcd.setTextColor(ST77XX_RED);
        lcd.print("8");
        lcd.fillRect(273, 18, 36, 1, ST77XX_CYAN); // Divider
        lcd.fillRect(273, 18, 36, 24, ST77XX_RED); // Bottom part of "8"
    } else if (isArmed == "DISAR") {
        lcd.setTextColor(ST77XX_CYAN);
        lcd.print("8");
        lcd.fillRect(298, 6, 24, 12, ST77XX_BLACK); // Clear top-right of "8"
        lcd.fillRect(273, 18, 36, 24, ST77XX_CYAN); // Bottom part of "8"
        lcd.fillRect(273, 18, 36, 1, ST77XX_BLACK); // Clear divider
    }

    // Display the state text below the icon
    lcd.setCursor(260, 43);
    lcd.setTextSize(2);
    lcd.print(isArmed);

    lastIsArmed = isArmed; // Update the last state
}

// Function to add a message to the log
void addToLog(const String& message) {
  // Shift existing logs down
  for (int i = logSize - 1; i > 0; i--) {
    lastDetectionLog[i] = lastDetectionLog[i - 1];
  }
  // Add the new message at the top
  lastDetectionLog[0] = message;
}

// Function to parse and format the MQTT message
String parseAndFormatMQTTMessage(const String& message) {
  // Find the positions of the delimiters in the original message
  int pos1 = message.indexOf('|');
  int pos2 = message.indexOf('|', pos1 + 1);
  int pos3 = message.indexOf('|', pos2 + 1);

  // Ensure the positions are valid
  if (pos1 == -1 || pos2 == -1 || pos3 == -1) {
    return "Parsing Error: Invalid Message Format";
  }

  // Extract parts of the message
  String clientID = message.substring(0, pos1);  // substring returns a String
  clientID.trim();                               // Ensure no extra spaces

  String detectedStatus = message.substring(pos1 + 1, pos2);
  detectedStatus.trim();

  String armedStatus = message.substring(pos2 + 1, pos3);
  armedStatus.trim();

  String dateTime = message.substring(pos3 + 1);
  dateTime.trim();

  // Convert armed/disarmed status to shorthand
  String armedShort = (armedStatus == "ARMED") ? "A" : "D";

  // Update isArmed here too
  if (armedShort == "A"){
    isArmed = "ARMED";
  } else if (armedShort == "D"){
    isArmed = "DISAR";
  } else {
    isArmed = " N/A ";
  }

  // Combine the reformatted string
  String formattedMessageLogging = dateTime + " " + armedShort + " " + clientID + " " + totalRetaliationCount; // USED FOR EXTERNAL LOGGING IN THE FUTURE
  String formattedMessage = dateTime + " " + armedShort; // USED FOR LCD
  return formattedMessage;
}

void updateShockTimeSinceBoot() {
    static String lastFormattedTime = ""; // Track the last displayed time

    // Calculate total seconds based on detections and convert to HHH:MM
    long totalSeconds = totalRetaliationCount * 4; // Multiply by 4 seconds per detection
    long hours = totalSeconds / 3600;             // Convert seconds to hours
    long minutes = (totalSeconds % 3600) / 60;    // Extract remaining minutes

    // Format the time as HHH:MM
    char timeBuffer[10];
    if (hours < 100) {
        // For less than 100 hours, include a leading space and two leading zeros if needed
        snprintf(timeBuffer, sizeof(timeBuffer), " %02ld:%02ld", hours, minutes);
    } else {
        // For 100 hours or more, display as 3 digits without a leading space
        snprintf(timeBuffer, sizeof(timeBuffer), "%03ld:%02ld", hours, minutes);
    }

    String formattedTime = String(timeBuffer);

    // Debugging output to verify calculations
    Serial.println("Total Detections: " + String(totalRetaliationCount));
    Serial.println("Total Seconds: " + String(totalSeconds));
    Serial.println("Formatted Time: " + formattedTime);

    // Check if the time has changed to avoid redundant updates
    if (formattedTime == lastFormattedTime) {
        return; // Exit if the time has not changed
    }

    // Update the display
    lcd.fillRect(190, 0, 70, 14, ST77XX_BLACK);  // Clear timer area
    lcd.setCursor(190, 0);                       // Set cursor position for timer
    lcd.setTextSize(2);                          // Set text size for timer
    lcd.setTextColor(ST77XX_MAGENTA);
    lcd.println(formattedTime);                  // Display the formatted time

    lastFormattedTime = formattedTime; // Update the last displayed time
}

void updateDateSinceBoot(){
  lcd.fillRect(190, 15, 70, 15, ST77XX_BLACK) ;  // Clear timer area
  lcd.setCursor(190, 15);                       // Set cursor position for timer
  lcd.setTextSize(2);                          // Set text size for timer
  lcd.setTextColor(ST77XX_MAGENTA);
  lcd.println(" " + dateSinceBoot);  // Display the timer
}

void buttonPressToggle() {
  static bool lastExternalButtonState = HIGH;  // Previous state of the external button
  static bool lastOnboardButtonState = HIGH;   // Previous state of the onboard button

  // Read the current state of both buttons
  bool currentExternalButtonState = digitalRead(EXTERNAL_BUTTON_PIN);
  bool currentOnboardButtonState = digitalRead(ONBOARD_BUTTON_PIN);

  // Check for external button press (HIGH -> LOW transition)
  if (lastExternalButtonState == HIGH && currentExternalButtonState == LOW) {
    sendRequest();
  }

  // Check for onboard button press (HIGH -> LOW transition)
  if (lastOnboardButtonState == HIGH && currentOnboardButtonState == LOW) {
    sendRequest();
  }

  // Update button states
  lastExternalButtonState = currentExternalButtonState;
  lastOnboardButtonState = currentOnboardButtonState;
}

void sendRequest() {
    // Recalculate the current time using elapsed millis
    time_t currentTime = internalTime + (millis() - lastMillis) / 1000; // Add elapsed seconds
    const int estOffset = -5 * 3600;  // Offset for EST (UTC-5)
    time_t estTime = currentTime + estOffset;  // Adjusted time for EST
    struct tm* estTimeInfo = gmtime(&estTime);

    // Format the date and time
    char dateBuffer[20];
    char time24Buffer[10];
    strftime(dateBuffer, sizeof(dateBuffer), "%m/%d", estTimeInfo);       // MM/DD
    strftime(time24Buffer, sizeof(time24Buffer), "%H:%M:%S", estTimeInfo); // HH:MM:SS (24-hour format)

    // Determine action
    String action = (isArmed == "DISAR") ? "REQUESTED ARM" : "REQUESTED DISARM";

    // Construct MQTT message
    String mqttMessage = String(clientID) + " | " + action + " | " + String(dateBuffer) + " " + String(time24Buffer);

    // Publish MQTT message
    if (client.connected()) {
        client.publish(mqtt_topic_CENTRAL_HUB, mqttMessage.c_str());
        
        // Display success indicator on LCD
        lcd.fillRect(260, 0, 60, 57, ST77XX_BLACK);
        lcd.setTextColor(ST77XX_GREEN);
        lcd.setCursor(274, -3);
        lcd.setTextSize(6);
        lcd.write(0x18); // Success icon
        lcd.setCursor(260, 43);
        lcd.setTextSize(2);
        lcd.print("SENT!");
    } else {
        // Display failure indicator on LCD
        lcd.fillRect(260, 0, 60, 57, ST77XX_BLACK);
        lcd.setTextColor(ST77XX_RED);
        lcd.setCursor(274, -3);
        lcd.setTextSize(6);
        lcd.write(0x21); // Failure icon
        lcd.setCursor(260, 43);
        lcd.setTextSize(2);
        lcd.print("FAIL!");
    }
}


// Function to parse and format CENTRAL-HUB messages
String parseAndFormatCentralHubMessage(const String& message) {
  // Find the positions of the delimiters in the original message
  int pos1 = message.indexOf('|');
  int pos2 = message.indexOf('|', pos1 + 1);
  int pos3 = message.lastIndexOf('|');

  // Ensure the positions are valid
  if (pos1 == -1 || pos2 == -1 || pos3 == -1) {
    return "Parsing Error: Invalid Message Format";
  }

  // Extract parts of the message
  String clientID = message.substring(0, pos1);      // Extract the Client ID
  clientID.trim();                                   // Trim whitespace

  String status = message.substring(pos1 + 1, pos2); // Extract the status
  status.trim();                                     // Trim whitespace

  String dateTime = message.substring(pos3 + 1);     // Extract the timestamp
  dateTime.trim();                                   // Trim whitespace

  // Parse the status for shorthand notation
  if (status == "ARM CONFIRMED") {
    status = "A"; // Short for Armed
    isArmed = "ARMED";
  } else if (status == "DISARM CONFIRMED") {
    status = "D"; // Short for Disarmed
    isArmed = "DISAR";
  }

  // Split date and time from the dateTime string
  int spacePos = dateTime.indexOf(' ');
  String date = "N/A";
  String time = "N/A";
  if (spacePos != -1) {
    date = dateTime.substring(0, spacePos);  // Extract full date
    String fullTime = dateTime.substring(spacePos + 1); // Extract time with seconds

    // Extract HH:MM from the full time (keeping logic for seconds)
    int colonPos = fullTime.indexOf(':');
    if (colonPos != -1) {
      int secondColonPos = fullTime.indexOf(':', colonPos + 1);
      if (secondColonPos != -1) {
        time = fullTime.substring(0, secondColonPos); // For HH:MM:SS
      } else {
        time = fullTime.substring(0, colonPos + 3); // For HH:MM
      }
    }

    // Reformat the date to exclude the year, keeping only MM/DD
    int slashPos = date.indexOf('/');
    if (slashPos != -1) {
      int secondSlashPos = date.indexOf('/', slashPos + 1);
      if (secondSlashPos != -1) {
        date = date.substring(0, secondSlashPos); // Trim to MM/DD
      }
    }
  }

  // Update global variables for LCD display
  lastRequestClientID = clientID;
  lastRequestStatus = status;
  lastRequestDate = date;
  lastRequestTime = time; // HH:MM format

  // Combine the formatted string
  return date + " " + time + " | " + status + " | " + clientID;
}


//onboard button to shutdown lcd or not
//add touch sensor to either arm or disarm
//add external logging/
//cp437
