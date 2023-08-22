#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

#define TINY_GSM_MODEM_SIM7600



#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200

#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]      = "Vina";
const char gprsUser[] = "mms";
const char gprsPass[] = "mms";

#include <TinyGsmClient.h>
#include <PubSubClient.h>
// MQTT details
const char* broker = "513booyoungct4.ddns.net";

const char* topicLed       = "GsmClientTest/led";
const char* topicInit      = "GsmClientTest/init";
const char* topicLedStatus = "GsmClientTest/ledStatus";
static const int RXPin = 19, TXPin = 18;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);


TinyGsm modem(Serial2);
TinyGsmClient client(modem);
PubSubClient  mqtt(client);

#define LED_PIN 13
int ledStatus = LOW;

uint32_t lastReconnectAttempt = 0;
void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();

  // Only proceed if incoming message's topic matches
  if (String(topic) == topicLed) {
    ledStatus = !ledStatus;
    digitalWrite(LED_PIN, ledStatus);
    mqtt.publish(topicLedStatus, ledStatus ? "1" : "0");
  }

}
long time_wifi = 0;
void reconnect() {
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect("GPS_Device1", "tyne", "123456")) {
      Serial.println("connected");
      // Subscribe
      mqtt.publish("GsmClientTest/init", "Connect Success");
      delay(2000);
      mqtt.publish("esp32/status", "Connect Failed");
    }
    else {
      long now = millis();
      if (now - time_wifi > 15000)
      {
        setup_GPRS();
        time_wifi = now;
        delay(5000);
      }
      Serial.print("failed, err_code:");
      Serial.println(mqtt.state());
      delay(100);
      Serial.println(" try again in 5 seconds");
      mqtt.disconnect();
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void setup() {
  // Set console baud rate
  Serial.begin(115200);
  ss.begin(GPSBaud);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);

  // !!!!!!!!!!!
  // Set your reset, enable, power pins here
  // !!!!!!!!!!!

  Serial.println("Wait...");
  Serial2.begin(115200);
  delay(6000);
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  modem.restart();
  // modem.init();
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);
  delay(2000);
  //if (GSM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(GSM_PIN); }
  setup_GPRS();
  mqtt.setServer(broker, 1893);
  mqtt.setCallback(mqttCallback);

}
void setup_GPRS() {
  delay(100);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  modem.gprsConnect(apn, gprsUser, gprsPass);
  delay(1000);
  uint8_t count = 0;
  while (!modem.isNetworkConnected()) {
    Serial.print(".");
    delay(500);
    count ++;
    if (count > 30) ESP.restart();
  }
  Serial.println(" success");
}
void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();
  StaticJsonDocument<256> doc1;
  if (ss.available() > 0)
    if (gps.encode(ss.read()))
    {

      doc1["latitute"] = gps.location.lat();
      doc1["longtitute"] = gps.location.lng();
    }
//
//  if (millis() > 5000 && gps.charsProcessed() < 10)
//  {
//    Serial.println(F("No GPS detected: check wiring."));
//    while (true);
//  }
//  delay(1000);
//  doc1["latitute"] = gps.location.lat();
//  doc1["longtitute"] = gps.location.lng();

  Serial.println("###############################################");

  String Jdata;
  serializeJson(doc1, Jdata);
  //  //gửi json đến topic esp32/json
  mqtt.publish("v1/devices/me/telemetry", Jdata.c_str());
  Serial.println("MQTT Send Data");
  delay(2000);

}
void displayInfo()
{
  Serial.print(F("Location: "));
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}
