#include <ld2410.h>

ld2410 radar;
uint32_t lastReading = 0;

// Sử dụng chân 16 (RX) và 17 (TX) cho Serial2 của ESP32
#define RX_PIN 16
#define TX_PIN 17

void setup() {
  Serial.begin(115200);
  delay(1000); // Đợi Serial máy tính ổn định
  
  Serial.println("\nLD2410 test started - ESP32 Hardware Serial");

  // BƯỚC QUAN TRỌNG: Bắt buộc phải begin Serial2 trước khi gọi radar.begin()
  Serial2.begin(256000, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(500); // Đợi khởi tạo cổng Serial2

  if (radar.begin(Serial2)) {
    Serial.println("Kết nối cảm biến THÀNH CÔNG!");
  } else {
    Serial.println("LỖI: Không tìm thấy cảm biến LD2410C.");
  }
}

void loop() {
  // Yêu cầu thư viện đọc dữ liệu liên tục
  radar.read();
  
  // Chỉ in ra màn hình 1 giây / lần để tránh treo Serial Monitor
  if (radar.isConnected() && millis() - lastReading > 1000) {
    lastReading = millis();
    
    if (radar.presenceDetected()) {
      Serial.println("CÓ NGƯỜI");
    } else {
      Serial.println("KHÔNG CÓ AI");
    }
  }
}