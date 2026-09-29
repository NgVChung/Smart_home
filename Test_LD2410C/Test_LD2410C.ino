#include <ld2410.h>

ld2410 radar;
uint32_t lastReading = 0;

// Khai báo chân cho UART2 trên ESP32
#define RX_PIN 16
#define TX_PIN 17

void setup() {
  // Khởi tạo Serial Monitor để xem kết quả trên máy tính
  Serial.begin(115200);
  
  // Khởi tạo Serial2 giao tiếp với LD2410C (Baud rate mặc định là 256000)
  Serial2.begin(256000, SERIAL_8N1, RX_PIN, TX_PIN);
  
  delay(1000);
  Serial.println("\nĐang kết nối với cảm biến LD2410C...");

  // Bắt đầu kết nối với cảm biến
  if (radar.begin(Serial2)) {
    Serial.println("Kết nối thành công!");
  } else {
    Serial.println("Lỗi: Không tìm thấy cảm biến. Vui lòng kiểm tra lại dây nối.");
  }
}

void loop() {
  // Liên tục đọc dữ liệu từ Serial2
  radar.read();
  
  // Hiển thị kết quả mỗi giây một lần
  if (radar.isConnected() && millis() - lastReading > 1000) {
    lastReading = millis();
    
    // Kiểm tra xem có người/vật thể trong vùng quét không
    if (radar.presenceDetected()) {
      
      // Nếu có mục tiêu đang đứng yên (Stationary target)
      if (radar.stationaryTargetDetected()) {
        Serial.print("Mục tiêu ĐỨNG YÊN - Khoảng cách: ");
        Serial.print(radar.stationaryTargetDistance());
        Serial.print(" cm | Năng lượng: ");
        Serial.println(radar.stationaryTargetEnergy());
      }
      
      // Nếu có mục tiêu đang chuyển động (Moving target)
      if (radar.movingTargetDetected()) {
        Serial.print("Mục tiêu DI CHUYỂN - Khoảng cách: ");
        Serial.print(radar.movingTargetDistance());
        Serial.print(" cm | Năng lượng: ");
        Serial.println(radar.movingTargetEnergy());
      }
      
      Serial.println("-----------------------------------");
    } else {
      Serial.println("Không có ai trong khu vực.");
    }
  }
}