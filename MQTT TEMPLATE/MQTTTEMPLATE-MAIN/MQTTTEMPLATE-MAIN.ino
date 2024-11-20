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

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

// WORKOUT-TIMER VARIABLES
String WORKOUT_TIMER_TIME = "";

// Wi-Fi credentials
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
const char* clientID = "RESV-MAIN"; // Change the NAME/NUMBER for each device******************

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
  // Print the received message for all topics
  Serial.print(String(clientID) + " RCVD [");
  Serial.print(topic);
  Serial.print("]: ");

  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // RCV TIMER MQTT 
  if (String(topic) == "WORKOUT-TIMER") {
    // Parse the "MM:SS" format
    int colonIndex = message.indexOf(':');
    if (colonIndex != -1) { // Ensure the colon exists
      int WORKOUT_TIMER_MINUTES = message.substring(0, colonIndex).toInt();    // Extract minutes
      int WORKOUT_TIMER_SECONDS = message.substring(colonIndex + 1).toInt();  // Extract seconds
      String WORKOUT_TIMER_TIME = String(WORKOUT_TIMER_MINUTES) + ":" + String(WORKOUT_TIMER_SECONDS);
      
      // Display parsed time
      Serial.println("Parsed Time: Minutes: " + String(WORKOUT_TIMER_MINUTES) + " Seconds: " + String(WORKOUT_TIMER_SECONDS) + " WORKOUT_TIMER_TIME: " + WORKOUT_TIMER_TIME);
      Serial.println("WORKOUT_TIMER_TIME:" + WORKOUT_TIMER_TIME);
     
      lcd.setRotation(1);
      lcd.fillScreen(ST77XX_BLACK);
      lcd.setCursor(0, 0);
      lcd.setTextSize(4);
      lcd.setTextColor(ST77XX_WHITE);
      lcd.print(WORKOUT_TIMER_TIME);



      // Add logic here to update your display or perform actions
    } else {
      Serial.println("Invalid time format received.");
    }
  } 
  else if (String(topic) == "another-topic") {
    // Handle messages for another topic
    Serial.println("Processing message for another-topic...");
    // Add custom logic for this topic
  } 
  else {
    // Default action for unrecognized topics
    Serial.println("Message received for unhandled topic.");
  }
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

        Serial.println(String(clientID) + " | SUBSCRIBED TO : [" + mqtt_topic_CENTRAL_HUB + "] | [" + mqtt_topic_NTP + "] | [ " + mqtt_topic_WORKOUT_TIMER + "]");
        //Serial.println(String(clientID) + " | [ ONLY PUBLISHING TO " + mqtt_topic_WORKOUT_TIMER + "]");

      } else {
        Serial.print(String(clientID) + " | MQTT BROKER CONNECTION FAILED!, rc=");
        Serial.println(client.state());
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();

  espClient.setCACert(root_ca);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  client.setKeepAlive(60); // Set keep-alive to 60 seconds

  //OPERATIONAL CODE
   //OPERATIONAL CODE
  lcd.init(LCD_WIDTH, LCD_HEIGHT);
  delay(500);
  lcd.setRotation(1);
  lcd.fillScreen(ST77XX_BLACK);

  lcd.setCursor(0, 0);
  lcd.setTextSize(3);
  lcd.setTextColor(ST77XX_WHITE);
  lcd.print(WORKOUT_TIMER_TIME);

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
