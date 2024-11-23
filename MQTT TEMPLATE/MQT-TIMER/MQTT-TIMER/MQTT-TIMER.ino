#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <time.h>

//OPERATIONAL CODE
#define LCD_WIDTH 170
#define LCD_HEIGHT 320
#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_CS 15
#define LCD_DC 2
#define LCD_RST 4
#define LCD_BLK 32

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

//OPERATIONAL CODE
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
bool buttonState = HIGH;
bool lastButtonState = HIGH;

// Threshold flags
bool threshold250msReached = false;
bool threshold500msReached = false;
bool threshold750msReached = false;

//CONNECTION CODE
//Wi-Fi credentials
const char* ssid = "icup +1"; // BE MINDFUL ******************
const char* password = "aaaaaaaaa1"; // v


// MQTT broker credentials
const char* mqtt_server = "9321cdfa0af34b83b77797a4488354cd.s1.eu.hivemq.cloud"; // BE MINDFUL ******************
const int mqtt_port = 8883; // BE MINDFUL ******************
const char* mqtt_user = "MasterA"; // BE MINDFUL ******************
const char* mqtt_password = "MasterA1"; // BE MINDFUL ******************
const char* mqtt_topic_CENTRAL_HUB = "CENTRAL-HUB"; // Add if more topics ******************
const char* mqtt_topic_WORKOUT_TIMER = "WORKOUT-TIMER"; // Add if more topics ******************
const char* mqtt_topic_NTP = "NTP"; // Add if more topics ******************
const char* clientID = "RESV-TIMER"; // Change the NAME/NUMBER for each device******************


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
#define MAX_NTP_RETRIES 5
int NTPReadyToPublish = 0;

// Internal time tracking
time_t internalTime = 0; // Tracks the current epoch time
unsigned long lastMillis = 0; // Tracks the last time the display was updated

// Connect to Wi-Fi
void setup_wifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected! IP: " + WiFi.localIP().toString());
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
  Serial.println("NTP Time fetched: " + String(ctime(&internalTime)));

  // Publish updated times after fetch
  publishTimeData();

  return true;
}

// Display and format all timezone data
void displayTimezones() {
  const int offsets[] = {0, -5, -6, -7, -8};
  const char* zones[] = {"UTC-0", "EST-5", "CST-6", "MST-7", "PST-8"};
  char dateBuffer[20];
  char time12Buffer[10];
  char time24Buffer[10];

  lcd.fillScreen(ST77XX_BLACK);
  lcd.setCursor(0, 0);
  lcd.setTextSize(2);
  lcd.setTextColor(ST77XX_WHITE);

  for (int i = 0; i < 5; i++) {
    time_t adjustedTime = internalTime + (offsets[i] * 3600);
    struct tm* timeInfo = gmtime(&adjustedTime);

    strftime(dateBuffer, sizeof(dateBuffer), "%m-%d", timeInfo);
    strftime(time12Buffer, sizeof(time12Buffer), "%I:%M %p", timeInfo);
    strftime(time24Buffer, sizeof(time24Buffer), "%H:%M", timeInfo);

    lcd.println(String(zones[i]) + " | " + String(dateBuffer) + " | " +
                String(time12Buffer) + " | " + String(time24Buffer));
  }
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

  //client.publish(mqtt_topic_NTP, payload.c_str()); (DOESN"T WORK DUE TO HIVEMQ MQTT CHAR LIMIT?)
  Serial.println("----------- NTP TIME -----------\n" + payload);
  
  // Publish only EST-5 row separately
  int estIndex = 1; // Index of EST-5 in the offsets array
  time_t estTime = internalTime + (offsets[estIndex] * 3600);
  struct tm* estTimeInfo = gmtime(&estTime);

  strftime(dateBuffer, sizeof(dateBuffer), "%m/%d", estTimeInfo);
  strftime(time12Buffer, sizeof(time12Buffer), "%I:%M %p", estTimeInfo);
  strftime(time24Buffer, sizeof(time24Buffer), "%H:%M", estTimeInfo);

  String estPayload = String(zones[estIndex]) + " | " + String(dateBuffer) + " | " +
                      String(time12Buffer) + " | " + String(time24Buffer);

  Serial.println("Staged NTP Publish data:");
  Serial.println(estPayload + "\n");

  if (NTPReadyToPublish == 1){
    client.publish(mqtt_topic_NTP, estPayload.c_str());
  } else {
    Serial.println("Staged NTP could not publish, NTPReadyToPublish flag remains at 0\n");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print(String(clientID) + " RCVD [");
  Serial.print(topic);
  Serial.print("]: ");
  
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  static unsigned long lastAttemptTime = 0;
  const unsigned long retryInterval = 5000;

  if (!client.connected()) {
    if (millis() - lastAttemptTime > retryInterval) {
      lastAttemptTime = millis();
      Serial.print(String(clientID) +" | ATTEMPTING TO CONNECT TO MQTT BROKER...\n");
      if (client.connect(clientID, mqtt_user, mqtt_password)) {
        Serial.println(String(clientID) + " | CONNECTED TT MQTT BROKER!");

        client.subscribe(mqtt_topic_CENTRAL_HUB);
        client.publish(mqtt_topic_CENTRAL_HUB, (String(clientID) + " CONNECTED").c_str());

        client.subscribe(mqtt_topic_NTP);
        //client.publish(mqtt_topic_NTP, (String(clientID) + " CONNECTED").c_str());
        NTPReadyToPublish = 1;

        //client.subscribe(mqtt_topic_WORKOUT_TIMER);
        //String workOutTimerConnect = String(clientID) + " | ONLY PUB TO WORKOUT-TIMER";
        //client.publish(mqtt_topic_WORKOUT_TIMER, workOutTimerConnect.c_str());

        Serial.println(String(clientID) + " | FULLY SUBSCRIBED TO [" + mqtt_topic_CENTRAL_HUB + "] | [" + mqtt_topic_NTP + "]");
        Serial.println(String(clientID) + " | ONLY PUBLISHING TO [" + mqtt_topic_WORKOUT_TIMER + "]");
      
      } else {
        Serial.println(" | MQTT BROKER CONNECTION FAILED!, rc=" + String(client.state()));
        delay(5000);
      }
    }
  }
}

//HYBRID CODE
void setup() {
  //CONNECTION CODE
  Serial.begin(115200);
  setup_wifi();

  espClient.setCACert(root_ca);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  //OPERATIONAL CODE
  lcd.init(LCD_WIDTH, LCD_HEIGHT);
  lcd.setRotation(1);
  lcd.fillScreen(ST77XX_BLACK);

  lcd.setCursor(0, 0);
  lcd.setTextSize(3);
  lcd.setTextColor(ST77XX_WHITE);
  lcd.print("TIMER STANDING BY BUTTON 12        BUZZER 27        LED 13");
  lcd.println("Fetching NTP...");
  lcd.println("Please wait...");

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  timeClient.begin();

  if (fetchAndSetNTPTime()) {
    lastNTPFetch = millis();
  } else {
    Serial.println("Using default time.");
    internalTime = 1321019471; // 11/11/11 11:11 UTC
  }

  displayTimezones();
}

void loop() {
  //CONNECTION CODE
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - lastMillis >= 60000) {
    internalTime += 60;
    displayTimezones();
    lastMillis = currentMillis;
  }

  if (millis() - lastNTPFetch >= SECONDS_IN_A_DAY * 1000) {
    if (fetchAndSetNTPTime()) {
      lastNTPFetch = millis();
    }
  }  

  //OPERATIONAL CODE
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
  if (threshold750msReached) return;  // Avoid repeating the reset

  noTone(buzzerPin);
  timerRunning = false;
  timerRemaining = timerInterval;

  lcd.fillRect(0, 0, LCD_HEIGHT, LCD_WIDTH, ST77XX_BLACK);
  lcd.setCursor(0, 0);
  lcd.setTextSize(6);
  lcd.setTextColor(ST77XX_BLUE);
  // place holder for LCD PRINT time to be defauled to --:-- until NTP MQTT is parsed. format of MM/DD HH:MM 
  Serial.println("----------- [ TIMER STOPPED ] ------------");

  String timeMessage = "--:--";
  client.publish(mqtt_topic_WORKOUT_TIMER, timeMessage.c_str());

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
      lcd.print("STOPPING TIME");
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
      lcd.print("STOPPING TIME");
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
      lcd.print("[==========");
      lcd.setTextSize(4);
      lcd.setCursor(5, 70);  // Position at the bottom row
      lcd.setTextColor(ST77XX_WHITE);
      lcd.print("STOPPING TIME");
      lcd.setTextSize(2);
      lcd.setCursor(180, 110);  // Position at the bottom row
      lcd.setTextColor(ST77XX_RED);
      lcd.print("==========]");
      Serial.println("HOLDING: 0.25 sec");
      
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

  // PUBLISH TO MQTT TOPIC WORKOUT-TIMER
  // Ensuring mm:ss format with leading zeros
  String timeMessage = "";
  if (minutes < 10) timeMessage += "0";
  timeMessage += String(minutes) + ":";
  if (seconds < 10) timeMessage += "0";
  timeMessage += String(seconds);
  client.publish(mqtt_topic_WORKOUT_TIMER, timeMessage.c_str());

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
  Serial.println("*** SEND MQTT TIMER FINISHED");

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
  

