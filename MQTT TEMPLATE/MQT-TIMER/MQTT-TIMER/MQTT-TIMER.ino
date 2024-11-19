#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

//OPERATIONAL CODE
#define LCD_WIDTH 170
#define LCD_HEIGHT 320
#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_CS 15
#define LCD_DC 2
#define LCD_RST 4
#define LCD_BLK 32

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

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

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

void setup_wifi() {
  Serial.println("-------------------------------------------------------------------");
  Serial.println("HOSTNAME | "+ String(clientID));
  
  Serial.println(String(clientID) + " | ATTEMPTING TO CONNECT TO WIFI...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  Serial.println(String(clientID) + " | CONNECTED TO WIFI!");
  Serial.print("IP ADDRESS | ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("MQTT RECEIVED [");
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
        String centralHubConnect = String(clientID) + " | CONNECTED TO CENTRAL-HUB";
        client.publish(mqtt_topic_CENTRAL_HUB, centralHubConnect.c_str());

        client.subscribe(mqtt_topic_NTP);
        String NTPConnect = String(clientID) + " | CONNECTED TO NTP-TOPIC";
        client.publish(mqtt_topic_NTP, NTPConnect.c_str());

        client.subscribe(mqtt_topic_WORKOUT_TIMER);
        String workOutTimerConnect = String(clientID) + " | CONNECTED TO WORKOUT-TIMER";
        client.publish(mqtt_topic_WORKOUT_TIMER, workOutTimerConnect.c_str());

        Serial.println(String(clientID) + " | SUBSCRIBED TO : [" + mqtt_topic_CENTRAL_HUB + "] | [" + mqtt_topic_NTP + "] | [" + mqtt_topic_WORKOUT_TIMER + "]");
      
      } else {
        Serial.print(String(clientID) + " | MQTT BROKER CONNECTION FAILED!, rc=");
        Serial.println(client.state());
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
  client.setKeepAlive(60);  // Set keep-alive to 60 seconds

  //OPERATIONAL CODE
  lcd.init(LCD_WIDTH, LCD_HEIGHT);
  delay(500);
  lcd.setRotation(1);
  lcd.fillScreen(ST77XX_BLACK);

  lcd.setCursor(0, 0);
  lcd.setTextSize(3);
  lcd.setTextColor(ST77XX_WHITE);
  lcd.print("TIMER STANDING BY BUTTON 12        BUZZER 27        LED 13");

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  Serial.begin(115200);
  Serial.println("TIMER STANDING BY...");

}

void loop() {
  //CONNECTION CODE
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

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
  noTone(buzzerPin);
  timerRunning = false;
  timerRemaining = timerInterval;

  lcd.fillRect(0, 0, LCD_HEIGHT, LCD_WIDTH, ST77XX_BLACK);
  lcd.setCursor(0, 0);
  lcd.setTextSize(6);
  lcd.setTextColor(ST77XX_BLUE);
  lcd.print("ADD STANDBY NTP");
  Serial.println("----------- [ TIMER SHUTDOWN ] -----------");
                  
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
      lcd.print("SHUTTING DOWN");
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
      lcd.print("SHUTTING DOWN");
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
      lcd.print("[==========]");
      lcd.setTextSize(4);
      lcd.setCursor(5, 70);  // Position at the bottom row
      lcd.setTextColor(ST77XX_WHITE);
      lcd.print("SHUTTING DOWN");
      lcd.setTextSize(2);
      lcd.setCursor(168, 110);  // Position at the bottom row
      lcd.setTextColor(ST77XX_RED);
      lcd.print("[==========]");
      Serial.println("HOLDING: 0.25 sec");
      lcd.fillRect(0, 0, LCD_HEIGHT, LCD_WIDTH, ST77XX_BLACK);
      
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
  

