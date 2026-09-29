// Khai báo chân GPIO kết nối với chân OUT/DATA của cảm biến PIR
// Bạn có thể thay đổi số 14 thành chân thực tế đang cắm trên mạch ESP32
const int PIR_PIN = 27; 

void setup() {
  // Khởi tạo Serial Monitor với tốc độ 115200 baud
  Serial.begin(115200);
  
  // Thiết lập chân PIR là đầu vào (Input)
  pinMode(PIR_PIN, INPUT);
  
  Serial.println("Dang khoi dong viec doc cam bien PIR...");
}

void loop() {
  // Đọc tín hiệu từ PIR (sẽ trả về 1 nếu có chuyển động, 0 nếu không có)
  int pirState = digitalRead(PIR_PIN);
  
  // In trạng thái ra màn hình
  Serial.println(pirState);
  
  // Thời gian trễ 100ms để tránh trôi màn hình quá nhanh nhưng vẫn đảm bảo tính liên tục
  delay(500); 
}