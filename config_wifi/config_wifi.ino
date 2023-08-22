#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Wire.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <EEPROM.h>

#define WIFI_SSID_ADDRESS               0u
#define WIFI_PASSWORD_ADDRESS           32u
#define CONFIG_BUTTON                   35u

String wifi_ssid = "";
String wifi_password = "";
bool isConfiging = false;
WebServer server(80);
LiquidCrystal_I2C LCD(0X27, 16, 2);
String st;
String content;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  LCD.init();
  LCD.backlight();
  EEPROM.begin(512);
  Serial.println("\n");
  Serial.println("Starting up");
  readEEPROM();
  setupWifi();
}
void setupWifi(void) {
  delay(10);
  uint8_t count;
  Serial.println("\nConnecting to: " + wifi_ssid);
  display(2, 5, "Connecting to ", "WIFI");
  /*EStablish wifi connection*/
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  while ((WiFi.status() != WL_CONNECTED) && (count < 20)) {
    delay(500);
    Serial.print(".");
    count ++;
    if (count > 20) {
      Serial.println("False to set up WiFi connection");
      display(0, 4, "Failed to connect", "to WIFI");
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    display(3, 2, "IP Address", WiFi.localIP().toString());
  }
}

void getConfig(void) {
  Serial.println("Starting config");
  display(0, 0, "Starting config", "" );
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
    content += "<p style=\"margin: 10px;\">Welcome to Home Automation System Configuration</p>";
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
  display(2, 2, "Please visit", WiFi.softAPIP().toString());
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
  WiFi.softAP("Home Automation System", "");
  Serial.println("Initializing SoftAP for WiFi config");
  //Start the webserver
  launchWeb();
  Serial.println("over");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
    isConfiging = true;
    getConfig();
  }
  Serial.println("Hello\n");
}
