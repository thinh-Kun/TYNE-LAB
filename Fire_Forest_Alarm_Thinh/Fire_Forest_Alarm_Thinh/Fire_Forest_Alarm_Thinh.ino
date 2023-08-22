// import library
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>
#include "DHT.h"
// Notify: Adafruit_ADS1X15.h modified

// define pin Button
#define DHTPIN 33
#define DHTTYPE DHT22
#define TimeUpdateParameter 5000
#define TimeUpdateFullScreen 62500
#define NameDevice "Mqtt Device 2"
#define NameDeviceText "D2"
#define posx "latitute"
#define posy "longtitute"
#define valuex 0
#define valuey 0

const char* ssid = "AP_M_2.4G";    //AP_M_2.4G
const char* password = "admin13589";    //admin13589
// Add your MQTT Broker IP address, example:
const char* mqtt_server = "513booyoungct4.ddns.net";
const int port_id = 1893;
const char* ssid_server = "tyne"; //tyne
const char* pass_server = "123456"; // 123456

StaticJsonDocument<256> JSONbuffer;
//JsonObject& JSONencoder = JSONbuffer.createObject();
JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

// TẠO BIẾN TRỮ DỮ LIỆU GỬI ĐI
unsigned char hexData[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79}; //Read the gas density command /Don't change the order
long lastMsg = 0;
long lastRestartDisplay = 0;
long lastUpdateDisplay = 0;
long time_send_data = 0;
long time_connect_wifi = 0;


// CALIB CURRENT

// create varible meter
float humidity = 0;
float temperature = 0;
char temperature_str[10];
char humidity_str[10];
long co2_value = 0;
long hi, lo, CO2;
uint8_t count_reset = 0;
uint8_t count_begin = 0;
bool FlagWifi = 0;
bool AutoWifi = 1;
bool ModeWifi = 1;


// CREATE VARIBLE SETTING

// CREATE FLAG SETTING


// update Menu function
void updateMenu() {


  long now = millis();
  if (now - lastUpdateDisplay > TimeUpdateParameter) {
    lcd.setCursor(9, 0);
    lcd.print("    ");
    lcd.setCursor(2, 0);
    lcd.print("   ");
    lcd.setCursor(4, 1);
    lcd.print("     ");
    lcd.setCursor(12, 1);
    lcd.print(" ");
    lcd.setCursor(8, 1);
    lcd.print(" ");
    delay(50);
    lcd.setCursor(2, 0);
    lcd.print(temperature_str);
    lcd.setCursor(5, 1);
    lcd.print(co2_value);
    lcd.setCursor(11, 0);
    lcd.print(humidity_str);
    lastUpdateDisplay = now;
  }
  long currenUpdateDisplay = millis();
  if (currenUpdateDisplay - lastRestartDisplay > TimeUpdateFullScreen) {

    lcd.clear();


    lcd.setCursor(2, 0);
    lcd.print(temperature_str);
    lcd.setCursor(5, 1);
    lcd.print(co2_value);
    lcd.setCursor(11, 0);
    lcd.print(humidity_str);

    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.setCursor(0, 1);
    lcd.print("Co2:");
    lcd.setCursor(6, 0);
    lcd.print("*C");
    lcd.setCursor(9, 0);
    lcd.print("H:");
    lcd.setCursor(15, 0);
    lcd.print("%");
    lcd.setCursor(9, 1);
    lcd.print("ppm");
    lcd.setCursor(13, 1);
    lcd.print(NameDeviceText);
    if (ModeWifi == 0) {
      lcd.setCursor(15, 1);
      lcd.print("!");
    }
    lastRestartDisplay = currenUpdateDisplay;
  }


}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial2.begin(9600);
  delay(500);
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.print("Id:");
  lcd.setCursor(4, 0);
  lcd.print(ssid);
  lcd.setCursor(0, 1);
  lcd.print("Pass:");
  lcd.setCursor(6, 1);
  lcd.print(password);
  delay(1000);
  lcd.clear();
  delay(50);
  lcd.setCursor(2, 0);
  lcd.print("Connecting to");
  lcd.setCursor(5, 1);
  lcd.print("WiFi");
  ModeWifi = 1;
  count_begin = 0;
  setup_wifi();
  client.setServer(mqtt_server, port_id);
  client.setCallback(callback);
  updateMenu();
  xTaskCreatePinnedToCore(
    Read_ADC
    ,  "Read_And_Send_Data"   // A name just for humans
    ,  4000  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  0  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL
    ,  1);
}



long time_restart = 0;  // TIME TO AUTO RESTART WHEN CAN'T CONNECT AFTER A PERIOD OF TIME
void setup_wifi() {   // CONNECT TO WIFI
  if (AutoWifi == 1) {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    WiFi.begin(ssid, password);
    delay(1000);
    uint8_t count = 0;

    while (WiFi.status() != WL_CONNECTED && count_begin != 1) {
      Serial.print(".");
      lcd.setCursor(15, 1);
      lcd.print("!");
      delay(250);
      lcd.setCursor(15, 1);
      lcd.print(" ");
      ModeWifi = 1;
      delay(250);
      count ++;
      if (count > 30) {
        Serial.println("False to set up WiFi connection");
        ModeWifi = 0;
        break;
        //ESP.restart();
      }
    }
    if (count_begin != 0) ESP.restart();
    if (count_begin == 0 && ModeWifi == 1)
    {
      lcd.clear();
      lcd.print("Mode Online!");

      delay(3000);
      lcd.clear();
      count_begin = 1;
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    }
    if  (ModeWifi == 0 && count_begin == 0) {
      lcd.clear();
      lcd.print("Mode Offline!");
      delay(3000);
      lcd.clear();
    }
  }
}
// RECEIVED MESSAGE IN SERVER
void callback(char* topic, byte * message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (messageTemp == "OnRelay1")  //COMMAND OF SERVER
  {
    //    stateRelay1 = 0;
  }
}

void xulyJson(String msgdata) {   // CONVERT JSON TO VARIBLE
  DynamicJsonDocument doc(1024);

  deserializeJson(doc, msgdata);
  JsonObject obj = doc.as<JsonObject>();
  //  setpoint_cc_cv_1 = float(obj["SETPOINT_CC_CV_1"]);
  //  ocv_chg_max_vol_1 = float(obj["OCV_CHG_MAX_VOL_1"]);
  Serial.println(msgdata.length());
}
// reconnect wifi when disconnect
long time_wifi = 0;   // time wait reconnect wifi
void reconnect() {
  // Loop until we're reconnected
  if (ModeWifi == 1) {
    while (!client.connected()) {

      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect(NameDevice, ssid_server, pass_server)) {
        Serial.println("connected");
        // Subscribe
        client.subscribe("esp32/output");
        client.publish("esp32/status", "Connect Success");
        delay(2000);
        client.publish("esp32/status", "Connect Failed");
      } else {
        long now = millis();
        if (now - time_wifi > 15000)
        {
          setup_wifi();
          time_wifi = now;
          delay(5000);
        }

        Serial.print("failed, err_code:");
        Serial.println(client.state());
        delay(100);
        Serial.println(" try again in 5 seconds");
        client.disconnect();
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    delay(100);
  }
}

unsigned long timer_led = 0;
unsigned long timer_warning = 0;

void loop() {
  humidity = dht.readHumidity();
  float temp = dht.readTemperature();
  temperature = round(temp * 100) / 100.0;
  sprintf(temperature_str, "%.1f", temperature);
  sprintf(humidity_str, "%.1f", humidity);
  co2_value = CO2;    // co2_value: 1 -> có CO2 ; 0 -> ko có Co2
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.setCursor(0, 1);
  lcd.print("Co2:");
  lcd.setCursor(6, 0);
  lcd.print("*C");
  lcd.setCursor(9, 0);
  lcd.print("H:");
  lcd.setCursor(15, 0);
  lcd.print("%");
  lcd.setCursor(9, 1);
  lcd.print("ppm");
  lcd.setCursor(13, 1);
  lcd.print(NameDeviceText);
  if (ModeWifi == 0) {
    lcd.setCursor(15, 1);
    lcd.print("!");
  }

  //  lcd.setCursor(8, 1);
  //  lcd.print("L2:");
  updateMenu();
}

// THIS IS A TASK READ_ADC AUTO
void Read_ADC(void *pvParameters)
{
  (void) pvParameters;

  /*
    Blink
    Turns on an LED on for one second, then off for one second, repeatedly.

    If you want to know what pin the on-board LED is connected to on your ESP32 model, check
    the Technical Specs of your board.
  */


  for (;;) // A Task shall never return or exit.
  {
    // READ sensor

    Serial2.write(hexData, 9);
    for (int i = 0, j = 0; i < 9; i++)
    {
      if (Serial2.available() > 0)
      {
        int ch = Serial2.read();

        if (i == 2) {
          hi = ch;    //High concentration
        }
        if (i == 3) {
          lo = ch;    //Low concentration
        }
        if (i == 8) {
          CO2 = hi * 256 + lo; //CO2 concentration
        }
      }
    }


    if (ModeWifi == 1) {
      if (!client.connected()) {
        reconnect();
      }
      client.loop();
      long now = millis();
      if (now - lastMsg > 3000) {
        lastMsg = now;
        client.subscribe("esp32/doAn1/control");
        delay(10);
        if (now - time_send_data > 3000)
        {
          StaticJsonDocument<256> doc1;
          doc1["co2_value"] = co2_value;
          doc1["humidity"] = humidity;
          doc1["temperature"] = temperature_str;
          doc1[posx] = valuex;
          doc1[posy] = valuey;

          Serial.print("co2_value: "); Serial.println(co2_value);
          Serial.print("humidity: "); Serial.println(humidity);
          Serial.print("temperature: "); Serial.println(temperature);
          Serial.println("###############################################");

          String Jdata;
          serializeJson(doc1, Jdata);
          //gửi json đến topic esp32/json
          client.publish("v1/devices/me/telemetry", Jdata.c_str());
          Serial.println("MQTT Send Data");
          time_send_data = now;
        }
      }
    }

    vTaskDelay(1000);
  }
}
//THIS IS A TASK AUTO SEND DATA AFTER A PERIOD OF TIME
