#include <SoftwareSerial.h>

#define RX_PIN 16 // Chân Rx của cảm biến kết nối với chân D16 trên ESP32
#define TX_PIN 17 // Chân Tx của cảm biến kết nối với chân D17 trên ESP32

SoftwareSerial mhzSerial(RX_PIN, TX_PIN);

void setup() {
  Serial.begin(9600);
  mhzSerial.begin(9600);
  Serial.print("Begin");
}

void loop() {
  if (mhzSerial.available() >= 9) {
    byte data[9];
    for (int i = 0; i < 9; i++) {
      data[i] = mhzSerial.read();
    }

    // Kiểm tra tính hợp lệ của dữ liệu
    if (data[0] == 0xFF && data[1] == 0x86) {
      int CO2 = (int)data[2] * 256 + (int)data[3]; // Giá trị CO2 được gửi từ byte thứ 2 và 3
      int checksum = 0;
      for (int i = 1; i < 8; i++) {
        checksum += data[i];
      }
      checksum = 0xFF - checksum + 1;

      if (data[8] == checksum) {
        // In giá trị CO2 lên Serial Monitor
        Serial.print("Giá trị CO2: ");
        Serial.print(CO2);
        Serial.println(" ppm");
      }
    }
  }
}
