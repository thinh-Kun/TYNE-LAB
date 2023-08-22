#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define DHTPIN 33          // Chân D33
#define DHTTYPE DHT22      // Loại cảm biến DHT22

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Địa chỉ I2C của màn hình LCD: 0x27

const char* ssid = "AP_M_2.4G";
const char* password = "admin13589";
const char* mqttServer = "513booyoungct4.ddns.net";
const int mqttPort = 1893;
const char* mqttUsername = "zmkch3zrdenf8q06y018"; // Thay thế bằng tên đăng nhập MQTT của bạn
const char* mqttPassword = "zqxs30um7fh5ehp8bk7t"; // Thay thế bằng mật khẩu MQTT của bạn

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  lcd.print("DHT22 Example");
  
  dht.begin();

  // Kết nối WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Kết nối MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  while (!client.connected()) {
    if (client.connect("5anxzbqmo9ae5uotgidx", mqttUsername, mqttPassword)) {
      Serial.println("Connected to MQTT Broker");
    } else {
      Serial.print("Failed to connect to MQTT Broker, rc=");
      Serial.print(client.state());
      Serial.println(" Retry in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  client.loop(); // Đảm bảo duy trì kết nối MQTT

  delay(2000);

  float temperature = dht.readTemperature();  // Đọc nhiệt độ (°C)
  float humidity = dht.readHumidity();        // Đọc độ ẩm (%)

  if (isnan(temperature) || isnan(humidity)) {
    lcd.clear();
    lcd.print("Sensor Error");
    return;
  }

  lcd.clear();
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(humidity);
  lcd.print(" %");

  // Gửi dữ liệu đến Thingsboard qua MQTT
  char payload[50];
  snprintf(payload, sizeof(payload), "{\"temperature\": %.2f, \"humidity\": %.2f}", temperature, humidity);
  client.publish("v1/devices/me/telemetry", payload);
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Xử lý callback khi nhận dữ liệu từ MQTT Broker (nếu cần)
}
