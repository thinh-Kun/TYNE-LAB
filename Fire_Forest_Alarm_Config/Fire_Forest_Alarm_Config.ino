#include <ArduinoJson.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <EEPROM.h>
#include <math.h>
#include "DHT.h"

#define WIFI_SSID_ADDRESS               0u
#define WIFI_PASSWORD_ADDRESS           32u
#define CONFIG_BUTTON                   35u

#define DHTPIN 33
#define DHTTYPE DHT22
#define TimeUpdateParameter 5000
#define TimeUpdateFullScreen 62500
#define TimeUpdateIp 2000
#define NameDevice "Mqtt Device 5"
String NameDeviceText = "D5";
#define posx "latitute"
#define posy "longtitute"
#define valuex 0
#define valuey 0

const char* mqtt_server = "513booyoungct4.ddns.net";
const int port_id = 1893;
const char* ssid_server = "tyne"; //tyne
const char* pass_server = "123456"; // 123456
String wifi_ssid = "";
String wifi_password = "";

bool isConfiging = false;
String st;
String content;

StaticJsonDocument<256> JSONbuffer;
JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);
LiquidCrystal_I2C LCD(0X27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

// TẠO BIẾN TRỮ DỮ LIỆU GỬI ĐI
unsigned char hexData[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79}; //Read the gas density command /Don't change the order
unsigned long lastMsg = 0;
unsigned long lastRestartDisplay = 0;
unsigned long lastUpdateDisplay = 0;
unsigned long lastUpdateRestartIpScreen = 0;
unsigned long time_send_data = 0;
unsigned long time_connect_wifi = 0;

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

void updateMenu() {

  bool FlagUpdateIP = 0;
  long now = millis();
  if (now - lastUpdateDisplay > TimeUpdateParameter) {
    LCD.setCursor(11, 0);
    LCD.print("    ");
    LCD.setCursor(2, 0);
    LCD.print("   ");
    LCD.setCursor(4, 1);
    LCD.print("     ");
    LCD.setCursor(12, 1);
    LCD.print(" ");
    LCD.setCursor(7, 1);
    LCD.print("  ");
    LCD.setCursor(8, 0);
    LCD.print(" ");
    delay(50);
    LCD.setCursor(2, 0);
    LCD.print(temperature_str);
    LCD.setCursor(4, 1);
    LCD.print(co2_value);
    LCD.setCursor(11, 0);
    LCD.print(humidity_str);
    if (ModeWifi == 0) {
      
    
    delay(2000);
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print("WebFig IP:");
    LCD.setCursor(0, 1);
    LCD.print(WiFi.softAPIP());
    delay(1000);
    }
    lastUpdateDisplay = now;
  }
  //  if (ModeWifi == 0) {
  ////    long currenUpdateIpConfig = millis();
  ////    if (currenUpdateIpConfig - lastUpdateRestartIpScreen > TimeUpdateIp) {
  //      LCD.clear();
  //      LCD.setCursor(0, 0);
  //      LCD.print("WebFig IP:");
  //      LCD.setCursor(0, 1);
  //      LCD.print(WiFi.softAPIP());
  //      delay(1000);
  ////      lastUpdateRestartIpScreen = currenUpdateIpConfig;
  ////    }
  //  }

  long currenUpdateDisplay = millis();
  if (currenUpdateDisplay - lastRestartDisplay > TimeUpdateFullScreen) {

    LCD.clear();


    LCD.setCursor(2, 0);
    LCD.print(temperature_str);
    LCD.setCursor(4, 1);
    LCD.print(co2_value);
    LCD.setCursor(11, 0);
    LCD.print(humidity_str);

    LCD.setCursor(0, 0);
    LCD.print("T:");
    LCD.setCursor(0, 1);
    LCD.print("Co2:");
    LCD.setCursor(6, 0);
    LCD.print("*C");
    LCD.setCursor(9, 0);
    LCD.print("H:");
    LCD.setCursor(15, 0);
    LCD.print("%");
    LCD.setCursor(9, 1);
    LCD.print("ppm");
    LCD.setCursor(13, 1);
    LCD.print(NameDeviceText);
    if (ModeWifi == 0) {
      LCD.setCursor(15, 1);
      LCD.print("!");
    }
    else {
      LCD.setCursor(15, 1);
      LCD.print(" ");
    }
    lastRestartDisplay = currenUpdateDisplay;
  }


}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(9600);
  AutoWifi = 1;
  ModeWifi = 0;
  delay(500);
  dht.begin();
  LCD.init();
  LCD.backlight();
  LCD.setCursor(4, 0);
  LCD.print("TYNE LAB");
  delay(2000);
  EEPROM.begin(512);
  Serial.println("\n");
  Serial.println("Starting up");
  readEEPROM();
  setup_wifi();
  client.setServer(mqtt_server, port_id);
  client.setCallback(callback);
  xTaskCreatePinnedToCore(
    Read_ADC
    ,  "Read_And_Send_Data"   // A name just for humans
    ,  4000  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL
    ,  1);
}
void setup_wifi(void) {
  if (AutoWifi == 1) {
    delay(10);
    uint8_t count;
    Serial.println("\nConnecting to: " + wifi_ssid);
    display(0, 0, "Connecting to ... ", wifi_ssid.c_str());
    delay(1000);

    /*EStablish wifi connection*/
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
    while ((WiFi.status() != WL_CONNECTED) && (count < 20)) {
      delay(500);
      Serial.print(".");
      count ++;
      if (count > 20) {
        Serial.println("False to set up WiFi connection");
        AutoWifi = 0;
        LCD.clear();
      }
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      display(3, 2, "IP Address", WiFi.localIP().toString());
      ModeWifi = 1;
      AutoWifi = 0;
      LCD.clear();
    }
  }
}
long time_wifi = 0;   // time wait reconnect wifi
void reconnect() {
  // Loop until we're reconnected
  if (AutoWifi == 1) {
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
    LCD.clear();
    delay(100);
  }
}

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





void getConfig(void) {
  ModeWifi = 0;
  Serial.println("Turning the hotspot ON");
  //Create webserver for configuration
  setupAP();
  while (isConfiging == true) {
    Serial.print(".");
    delay(100);
    server.handleClient();
  }
  isConfiging = false;
}

void writeEEPROM(String index, uint16_t add) {
  uint16_t i;
  for (i = 0; i < index.length(); ++i) {
    EEPROM.write(i + add, index[i]);
    Serial.print("Wrote: ");
    Serial.println(index[i]);
  }
}

void readEEPROM(void) {
  //read ssid from EEPROM
  //const int ssid_add-> pass_add ->  mqtt_add -> mqtt_username_add -> mqtt_pass_ad -> mqtt_port_add -> pub_topic_add ->  sub_topic_add
  uint16_t i;
  Serial.println("Reading WiFi SSID from EEPROM");
  for (i = 0; i < WIFI_PASSWORD_ADDRESS; i++) {
    wifi_ssid += char(EEPROM.read(i));
  }
  Serial.println("SSID: " + wifi_ssid);
  //read password from EEPROM
  Serial.println("\nReading WiFi PASSWORD from EEPROM");
  for (i = WIFI_PASSWORD_ADDRESS; i < WIFI_PASSWORD_ADDRESS + 32; i++) {
    wifi_password += char(EEPROM.read(i));
  }
  Serial.println("PASSWORD: " + wifi_password);
}

void createWebServer() {
  uint16_t i;
  server.on("/", []() {
    IPAddress IP = WiFi.softAPIP();
    String IPString = IP.toString();

    //Create html message
    content = "<!DOCTYPE HTML><html><head><style>";
    content += ".list-box { border: 1px solid #ccc;border-radius: 5px;background-color: #f2f2f2;padding: 10px; width: 300px;}";
    content += ".list-box li {border-bottom: 1px solid #752525; padding: 3px 0px 3px 10px; font-size: 18px; list-style: none;}";
    content += ".list-box li.highlight {background-color: #5ad1f294;}";
    content += ".submit-box {margin-top: 10px; text-align: center; width: 300px;}";
    content += ".submit-box input[type=\"submit\"] {height: 40px; width: 100px; background-color: #3c9b41; color: #fff; border: none; border-radius: 4px; cursor: pointer; font-size: 20px;} ";
    content += ".submit-box input[type=\"submit\"]:hover {background-color: #86d188;}";
    content += ".config-box{ margin-top: 10px; border: 1px solid #ccc; border-radius: 5px; background-color: #f2f2f2; padding: 10px; width: 300px; }";
    content += ".data{ height: 30px; box-sizing: border-box; margin: 7px; }";
    content += "form label { display: inline-block; width: 90px; height: 100%;}";
    content += "form input { border: 1px solid #ccc; border-radius: 4px; width: 190px; height: 100%; box-sizing: border-box; padding-left: 10px; font-size: 15px;}";
    content += "p{ text-align: center;  font-size: 20px;  margin: 0;  font-weight: bold; width: 300px; }";
    content += "ol{ box-sizing: border-box; padding: 0px 10px 0px 0px; }";
    content += "body{ display: grid; place-items: center; }";
    content += "</style>";
    // Script
    content += "<script> function copyToSSID(li) { var ssidInput = document.getElementById('ssid'); ssidInput.value = li.innerHTML; var listItems = document.querySelectorAll('.list-box li'); listItems.forEach(function(item) { item.classList.remove('highlight'); }); li.classList.add('highlight'); } </script>";
    content += "</head>";

    content += "<body>";
    content += "<p style=\"margin: 10px;\">Welcome to Tyne Web Configurator</p>";
    content += "<div class=\"list-box\"> <p>Sellect network for ESP32</p>";
    // List scanned networks
    content += st;
    content += "</div>";
    // Config box
    content += "<form action=\"setting\" method=\"get\">";
    content += "<div class=\"config-box\"> <p>WIFI</p> <div class=\"data\"> <label>SSID </label> <input id=\"ssid\" name=\"ssid\" length=\"32\"> </div> <div class=\"data\"> <label>PASSWORD </label> <input id=\"password\" name=\"password\" length=\"64\">  </div> </div>";        // submit button
    content += "<div class = \"submit-box\"> <input type=\"submit\" value=\"SUBMIT\"> </div>";
    content += "</form> </body>";
    content += "</html>";
    server.send(200, "text/html", content);
  });

  server.on("/scan" , []() {
    IPAddress IP = WiFi.softAPIP();
    String IPString = IP.toString();

    content = "<!DOCTYPE HTML>\r\n<html>go back";
    server.send(200, "text/html", content);
  });

  server.on ("/setting", [] () {
    uint16_t i;
    String qssid = server.arg("ssid");
    String qpassword = server.arg("password");
    bool isUpdated = false;

    if (qssid.length() > 0) {
      isUpdated = true;
      Serial.println("Clearing WIFI from EEPROM");
      /*Clear EEPROM*/
      for (i = 0; i < WIFI_PASSWORD_ADDRESS + 32; ++i) {
        EEPROM.write(i, 0);
      }
      Serial.println("Received SSID: " + String(qssid));
      Serial.println("Received password: " +  String(qpassword));
      Serial.println("Writing new SSID to EEPROM");
      writeEEPROM(qssid, WIFI_SSID_ADDRESS);
      Serial.println("Writing new PASSWORD to EEPROM");
      writeEEPROM(qpassword, WIFI_PASSWORD_ADDRESS);
    }

    if (isUpdated) {
      EEPROM.commit();

      content = "{\"Success\":\"saved to EEPROM... Starting new connection\"}";
      display(6, 0, "DONE", "RESTARTING ESP32");
      ESP.restart();
    }
    else {
      content = "{\"Error\":\"404 not found\"}";
      Serial.println("Sending 404");
    }
    server.sendHeader("Access-Control_Allow_Origin", "*");
    server.send(404, "application/json", content);
  });
}
void display(int cursorLine0, int cursorLine1, String textLine0, String textLine1) {
  LCD.clear();
  LCD.setCursor(cursorLine0, 0);
  LCD.print(textLine0);
  LCD.setCursor(cursorLine1, 1);
  LCD.print(textLine1);
}
void launchWeb(void) {
  Serial.print("Please visit the Address: ");
  Serial.println(WiFi.softAPIP());

  createWebServer();

  server.begin();
  Serial.println("Server started");

}

void setupAP(void) {
  uint8_t i;
  uint8_t numberOfNetworks;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  numberOfNetworks = WiFi.scanNetworks();
  Serial.println("Done Scanning");
  if (numberOfNetworks == 0) {
    Serial.println("No network was found");
  }
  else {
    Serial.println(String(numberOfNetworks) + " networks was found");
    for ( i = 0; i < numberOfNetworks; i++) {
      Serial.println(String(i + 1) + ": " + WiFi.SSID(i));
    }
  }

  st = "<ol>"; // save scanned networks
  for (int i = 0; i < numberOfNetworks ; i++) {
    st += "<li onclick=\"copyToSSID(this)\">";
    st += WiFi.SSID(i);
    st += "</li>";
  }
  st += "</ol>";
  // Start the Wifi Access point
  String Ap_ssid = "Tyne_WebFig_" + NameDeviceText;
  WiFi.softAP(Ap_ssid.c_str(), "");
  Serial.println("Initializing SoftAP for WiFi config");
  //Start the webserver
  launchWeb();
  Serial.println("over");
}

unsigned long timer_led = 0;
unsigned long timer_warning = 0;

void loop() {

  // put your main code here, to run repeatedly:
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
    isConfiging = true;
    getConfig();
  }

}
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
    Serial.println("RTOS");
    delay(1000);
    humidity = dht.readHumidity();
    float temp = dht.readTemperature();
    temperature = round(temp * 100) / 100.0;
    sprintf(temperature_str, "%.1f", temperature);
    sprintf(humidity_str, "%.1f", humidity);
    co2_value = CO2;    // co2_value: 1 -> có CO2 ; 0 -> ko có Co2
    LCD.setCursor(0, 0);
    LCD.print("T:");
    LCD.setCursor(0, 1);
    LCD.print("Co2:");
    LCD.setCursor(6, 0);
    LCD.print("*C");
    LCD.setCursor(9, 0);
    LCD.print("H:");
    LCD.setCursor(15, 0);
    LCD.print("%");
    LCD.setCursor(9, 1);
    LCD.print("ppm");
    LCD.setCursor(13, 1);
    LCD.print(NameDeviceText);
    if (ModeWifi == 0) {
      LCD.setCursor(15, 1);
      LCD.print("!");
    }
    else {
      LCD.setCursor(15, 1);
      LCD.print(" ");
    }
    updateMenu();

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
