#include <Arduino.h>
#include "esp_sleep.h"

#define PIR_PIN 27

void setup()
{
    Serial.begin(115200);
    delay(1000);

    pinMode(PIR_PIN, INPUT);

    Serial.println();
    Serial.println("==========================");
    Serial.println(" ESP32 DEEP SLEEP TEST");
    Serial.println("==========================");

    // Đọc nguyên nhân wake-up
    esp_sleep_wakeup_cause_t reason =
        esp_sleep_get_wakeup_cause();

    if (reason == ESP_SLEEP_WAKEUP_EXT0)
    {
        Serial.println("WAKEUP: PIR");
    }
    else
    {
        Serial.println("BOOT: NORMAL");
    }

    // Đọc PIR hiện tại
    int pir = digitalRead(PIR_PIN);

    Serial.print("PIR = ");

    if (pir == HIGH)
    {
        Serial.println("HIGH");
    }
    else
    {
        Serial.println("LOW");
    }

    /*
     * Chỉ cho phép PIR HIGH đánh thức ESP32
     */
    esp_sleep_enable_ext0_wakeup(
        (gpio_num_t)PIR_PIN,
        1
    );

    /*
     * Chờ 5 giây để quan sát PIR
     */
    Serial.println("Doc PIR trong 5 giay...");

    unsigned long start = millis();

    while (millis() - start < 5000)
    {
        pir = digitalRead(PIR_PIN);

        Serial.print("PIR = ");
        Serial.println(pir);

        delay(500);
    }

    /*
     * Kiểm tra lại trước khi ngủ
     */
    pir = digitalRead(PIR_PIN);

    Serial.print("PIR truoc khi Sleep = ");
    Serial.println(pir);

    if (pir == LOW)
    {
        Serial.println("PIR dang LOW -> vao Deep Sleep");
        Serial.flush();

        esp_deep_sleep_start();
    }
    else
    {
        Serial.println(
            "PIR dang HIGH -> KHONG vao Sleep"
        );

        Serial.println(
            "Hay cho PIR tro ve LOW roi thu lai."
        );
    }
}

void loop()
{
}