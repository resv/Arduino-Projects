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

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

// Wi-Fi credentials
const char* ssid = "icup +1";
const char* password = "aaaaaaaaa1";

// MQTT broker credentials
const char* mqtt_server = "9321cdfa0af34b83b77797a4488354cd.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "MasterA";
const char* mqtt_password = "MasterA1";
const char* mqtt_topic_NTP = "NTP";
const char* clientID = "RESV-MAIN";

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
    delay(1000);
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

  strftime(dateBuffer, sizeof(dateBuffer), "%m-%d", estTimeInfo);
  strftime(time12Buffer, sizeof(time12Buffer), "%I:%M %p", estTimeInfo);
  strftime(time24Buffer, sizeof(time24Buffer), "%H:%M", estTimeInfo);

  String estPayload = String(zones[estIndex]) + " | " + String(dateBuffer) + " | " +
                      String(time12Buffer) + " | " + String(time24Buffer);

  client.publish(mqtt_topic_NTP, estPayload.c_str());
  Serial.println("Published EST-5 NTP data:");
  Serial.println(estPayload);
}

// Handle incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println("RESV-MAIN RCVD [" + String(topic) + "]: " + message);

  // Handle NTP requests
  if (String(topic) == mqtt_topic_NTP && message.endsWith("| NTP REQUEST")) {
    Serial.println("Processing NTP REQUEST...");
    if (fetchAndSetNTPTime()) {
      Serial.println("NTP updated and published.");
    }
  }
}

// Reconnect to MQTT broker
void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT broker...");
    if (client.connect(clientID, mqtt_user, mqtt_password)) {
      Serial.println("MQTT connected!");

      client.subscribe(mqtt_topic_NTP);
      client.publish(mqtt_topic_NTP, (String(clientID) + " CONNECTED").c_str());
    } else {
      Serial.println("MQTT connection failed, rc=" + String(client.state()));
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
  lcd.fillScreen(ST77XX_BLACK);

  lcd.setCursor(0, 0);
  lcd.setTextSize(2);
  lcd.setTextColor(ST77XX_WHITE);
  lcd.println("Fetching NTP...");
  lcd.println("Please wait...");

  timeClient.begin();

  if (fetchAndSetNTPTime()) {
    lastNTPFetch = millis();
  } else {
    Serial.println("Using default time.");
    internalTime = 1321019471; // 11/11/11 11:11 UTC
  }

  displayTimezones();
}

// Loop function
void loop() {
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
}
