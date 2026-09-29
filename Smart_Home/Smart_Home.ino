#include <Arduino.h>
#include <ld2410.h>
#include "esp_sleep.h"


// ======================================================
// PIN
// ======================================================

#define PIR_PIN         27

#define LD2410_RX_PIN   16
#define LD2410_TX_PIN   17

#define LED_PIN  18
#define FAN_PIN  25

#define LD2410_BAUDRATE 256000


// ======================================================
// UART + LD2410
// ======================================================

HardwareSerial LD2410Serial(2);

ld2410 radar;


// ======================================================
// FSM
// ======================================================

enum RoomState
{
    STATE_STARTUP,

    STATE_PIR_HIGH,
    STATE_PIR_LOW,

    STATE_OCCUPIED,
    STATE_NO_PRESENCE,

    STATE_SLEEP
};

RoomState currentState;


// ======================================================
// TIMER
// ======================================================

const unsigned long STABLE_TIME = 10000;

unsigned long stateStartTime;


// ======================================================
// BIẾN TRẠNG THÁI RADAR
// ======================================================

bool radarPresence = false;


// ======================================================
// KHAI BÁO HÀM
// ======================================================

void changeState(RoomState newState);

void processFSM();

void processLD2410();

void enterDeepSleep();

void printWakeupReason(); 

void releaseGPIOHold();

void controlDevices();

void prepareGPIOForSleep();


// ======================================================
// SETUP
// ======================================================

void setup()
{
    Serial.begin(115200);

    delay(500);

    Serial.println();
    Serial.println("================================");
    Serial.println("       ESP32 SMART ROOM");
    Serial.println("       PIR + LD2410C + FSM");
    Serial.println("================================");

    pinMode(LED_PIN, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);

    releaseGPIOHold();
    // --------------------------------------------------
    // PIR
    // --------------------------------------------------

    pinMode(PIR_PIN, INPUT);


    // --------------------------------------------------
    // LD2410C
    // --------------------------------------------------

    LD2410Serial.begin(
        LD2410_BAUDRATE,
        SERIAL_8N1,
        LD2410_RX_PIN,
        LD2410_TX_PIN
    );


    // Khởi tạo thư viện LD2410
    if (radar.begin(LD2410Serial))
    {
        Serial.println("[LD2410] Connected");
    }
    else
    {
        Serial.println("[LD2410] Connection failed");
    }


    // --------------------------------------------------
    // Kiểm tra nguyên nhân wake-up
    // --------------------------------------------------

    printWakeupReason();


    // --------------------------------------------------
    // Bắt đầu FSM
    // --------------------------------------------------

    changeState(STATE_STARTUP);
}


// ======================================================
// LOOP
// ======================================================

void loop()
{
    processFSM();
}


// ======================================================
// FSM
// ======================================================

void processFSM()
{
    int pirValue = digitalRead(PIR_PIN);


    switch (currentState)
    {

        // ==================================================
        // STARTUP
        // ==================================================

        case STATE_STARTUP:

            Serial.println("[FSM] STARTUP");

            if (pirValue == HIGH)
            {
                changeState(STATE_PIR_HIGH);
            }
            else
            {
                changeState(STATE_PIR_LOW);
            }

            break;


        // ==================================================
        // PIR HIGH
        // ==================================================

        case STATE_PIR_HIGH:

            if (pirValue == HIGH)
            {
                if (millis() - stateStartTime >= STABLE_TIME)
                {
                    Serial.println();
                    Serial.println("[FSM] PIR HIGH 10s");

                    Serial.println("[FSM] -> SLEEP");
                    Serial.println("[FSM] Wake when PIR LOW");

                    changeState(STATE_SLEEP);
                }
            }
            else
            {
                Serial.println("[FSM] PIR HIGH -> LOW");

                // PIR vừa chuyển HIGH -> LOW
                // Kiểm tra LD2410 xem còn người không

                processLD2410();

                if (radarPresence)
                {
                    changeState(STATE_OCCUPIED);
                }
                else
                {
                    changeState(STATE_NO_PRESENCE);
                }
            }

            break;


        // ==================================================
        // PIR LOW
        // ==================================================

        case STATE_PIR_LOW:

            // Luôn đọc LD2410
            processLD2410();


            // Nếu radar phát hiện người
            if (radarPresence)
            {
                changeState(STATE_OCCUPIED);
            }


            // Nếu PIR lại HIGH
            else if (pirValue == HIGH)
            {
                Serial.println("[FSM] PIR LOW -> HIGH");

                changeState(STATE_PIR_HIGH);
            }


            // Không có người + PIR LOW
            else
            {
                if (millis() - stateStartTime >= STABLE_TIME)
                {
                    Serial.println();
                    Serial.println("[FSM] PIR LOW 10s");
                    Serial.println("[FSM] No presence");

                    changeState(STATE_SLEEP);
                }
            }

            break;


        // ==================================================
        // OCCUPIED
        // ==================================================

        case STATE_OCCUPIED:

            // Đọc LD2410
            processLD2410();


            // Nếu radar vẫn thấy người
            if (radarPresence)
            {
                // Vẫn còn người
                // Không làm gì
            }


            // Radar không còn thấy người
            else
            {
                Serial.println("[FSM] Presence lost");

                changeState(STATE_NO_PRESENCE);
            }


            // Nếu PIR HIGH
            if (pirValue == HIGH)
            {
                // Có chuyển động
                // Vẫn ở OCCUPIED
            }

            break;


        // ==================================================
        // NO PRESENCE
        // ==================================================

        case STATE_NO_PRESENCE:

            // Đọc LD2410
            processLD2410();


            // Radar phát hiện lại người
            if (radarPresence)
            {
                Serial.println("[FSM] Radar detected person");

                changeState(STATE_OCCUPIED);
            }


            // PIR phát hiện chuyển động
            else if (pirValue == HIGH)
            {
                Serial.println("[FSM] PIR detected movement");

                changeState(STATE_PIR_HIGH);
            }


            // Không có người
            else
            {
                if (millis() - stateStartTime >= STABLE_TIME)
                {
                    Serial.println();
                    Serial.println("[FSM] NO PRESENCE 10s");

                    changeState(STATE_SLEEP);
                }
            }

            break;


        // ==================================================
        // SLEEP
        // ==================================================

        case STATE_SLEEP:

            enterDeepSleep();

            break;
    }
}


// ======================================================
// ĐỔI STATE
// ======================================================

void changeState(RoomState newState)
{
    currentState = newState;
    stateStartTime = millis();

    // Điều khiển thiết bị ngay khi đổi trạng thái
    controlDevices();

    switch (newState)
    {
        case STATE_STARTUP:
            Serial.println("[FSM] -> STARTUP");
            break;

        case STATE_PIR_HIGH:
            Serial.println("[FSM] -> PIR_HIGH");
            break;

        case STATE_PIR_LOW:
            Serial.println("[FSM] -> PIR_LOW");
            break;

        case STATE_OCCUPIED:
            Serial.println("[FSM] -> OCCUPIED");
            break;

        case STATE_NO_PRESENCE:
            Serial.println("[FSM] -> NO_PRESENCE");
            break;

        case STATE_SLEEP:
            Serial.println("[FSM] -> SLEEP");
            break;
    }
}

// ======================================================
// ĐỌC LD2410
// ======================================================

void processLD2410()
{
    // Thư viện xử lý dữ liệu UART
    radar.read();


    // Kiểm tra có người hay không
    radarPresence = radar.presenceDetected();


    Serial.print("[LD2410] Presence: ");

    if (radarPresence)
    {
        Serial.println("YES");
    }
    else
    {
        Serial.println("NO");
    }
}


// ======================================================
// DEEP SLEEP
// ======================================================

void enterDeepSleep()
{
    int pirValue = digitalRead(PIR_PIN);

    /*
     * Không tắt LED/FAN.
     * Giữ nguyên trạng thái hiện tại.
     */

    prepareGPIOForSleep();

    if (pirValue == HIGH)
    {
        Serial.println("[SLEEP] PIR = HIGH");
        Serial.println("[SLEEP] Wake-up level = LOW");

        esp_sleep_enable_ext0_wakeup(
            (gpio_num_t)PIR_PIN,
            0
        );
    }
    else
    {
        Serial.println("[SLEEP] PIR = LOW");
        Serial.println("[SLEEP] Wake-up level = HIGH");

        esp_sleep_enable_ext0_wakeup(
            (gpio_num_t)PIR_PIN,
            1
        );
    }

    Serial.println("[SLEEP] ESP32 -> DEEP SLEEP");

    Serial.flush();

    esp_deep_sleep_start();
}

// ======================================================
// WAKE-UP REASON
// ======================================================

void printWakeupReason()
{
    esp_sleep_wakeup_cause_t reason;

    reason = esp_sleep_get_wakeup_cause();


    switch (reason)
    {
        case ESP_SLEEP_WAKEUP_EXT0:

            Serial.println("[WAKEUP] Wake-up by PIR / EXT0");

            break;


        default:

            Serial.println("[WAKEUP] Normal startup");

            break;
    }
}
void controlDevices()
{
    switch (currentState)
    {
        case STATE_PIR_HIGH:
            digitalWrite(LED_PIN, HIGH);
            digitalWrite(FAN_PIN, HIGH);

            Serial.println("[DEVICE] LED = ON");
            Serial.println("[DEVICE] FAN = ON");
            break;

        case STATE_OCCUPIED:
            digitalWrite(LED_PIN, HIGH);
            digitalWrite(FAN_PIN, HIGH);

            Serial.println("[DEVICE] LED = ON");
            Serial.println("[DEVICE] FAN = ON");
            break;

        case STATE_NO_PRESENCE:
            digitalWrite(LED_PIN, LOW);
            digitalWrite(FAN_PIN, LOW);

            Serial.println("[DEVICE] LED = OFF");
            Serial.println("[DEVICE] FAN = OFF");
            break;

        case STATE_PIR_LOW:
            // Chưa có kết luận cuối cùng
            // Giữ nguyên trạng thái LED/FAN
            break;

        case STATE_SLEEP:
            // CỰC KỲ QUAN TRỌNG:
            // Không thay đổi LED/FAN
            break;

        case STATE_STARTUP:
        default:
            break;
    }
}
void prepareGPIOForSleep()
{
    // Cho phép GPIO giữ nguyên mức hiện tại
    gpio_hold_en((gpio_num_t)LED_PIN);
    gpio_hold_en((gpio_num_t)FAN_PIN);

    Serial.println("[SLEEP] LED GPIO HOLD");
    Serial.println("[SLEEP] FAN GPIO HOLD");
}
void releaseGPIOHold()
{
    gpio_hold_dis((gpio_num_t)LED_PIN);
    gpio_hold_dis((gpio_num_t)FAN_PIN);

    Serial.println("[WAKEUP] GPIO HOLD released");
}