#include <Arduino.h>
#include "esp_sleep.h"


/* =========================================================
 * PIN
 * ========================================================= */

#define PIR_PIN             27

// LD2410C UART
#define LD2410_RX_PIN       16
#define LD2410_TX_PIN       17

#define LD2410_BAUDRATE     256000


/* =========================================================
 * UART LD2410C
 * ========================================================= */

HardwareSerial LD2410Serial(2);


/* =========================================================
 * FSM STATE
 * ========================================================= */

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


/* =========================================================
 * TIMER
 * ========================================================= */

// PIR phải giữ nguyên trạng thái trong 10 giây
const unsigned long PIR_STABLE_TIME = 10000;

// Thời điểm bắt đầu trạng thái hiện tại
unsigned long stateStartTime;


/* =========================================================
 * LD2410
 * ========================================================= */

bool radarPresence = false;


/* =========================================================
 * FUNCTION PROTOTYPES
 * ========================================================= */

void changeState(RoomState newState);

void processFSM();

void processLD2410();

bool readLD2410();

void enterDeepSleep();

void printWakeupReason();


/* =========================================================
 * SETUP
 * ========================================================= */

void setup()
{
    Serial.begin(115200);

    delay(500);

    Serial.println();
    Serial.println("=================================");
    Serial.println(" ESP32 SMART ROOM FSM");
    Serial.println(" PIR + LD2410C");
    Serial.println("=================================");


    /* -----------------------------------------------------
     * PIR
     * ----------------------------------------------------- */

    pinMode(PIR_PIN, INPUT);


    /* -----------------------------------------------------
     * LD2410C UART
     * ----------------------------------------------------- */

    LD2410Serial.begin(
        LD2410_BAUDRATE,
        SERIAL_8N1,
        LD2410_RX_PIN,
        LD2410_TX_PIN
    );


    /* -----------------------------------------------------
     * Wake-up reason
     * ----------------------------------------------------- */

    printWakeupReason();


    /* -----------------------------------------------------
     * Start FSM
     * ----------------------------------------------------- */

    changeState(STATE_STARTUP);
}


/* =========================================================
 * LOOP
 * ========================================================= */

void loop()
{
    processFSM();
}


/* =========================================================
 * FSM
 * ========================================================= */

void processFSM()
{
    int pirValue = digitalRead(PIR_PIN);


    switch (currentState)
    {

        /* =================================================
         * STARTUP
         * ================================================= */

        case STATE_STARTUP:

            Serial.println();
            Serial.println("[FSM] STARTUP");

            /*
             * Sau khi ESP32 boot hoặc wake-up,
             * kiểm tra trạng thái hiện tại của PIR.
             */

            if (pirValue == HIGH)
            {
                changeState(STATE_PIR_HIGH);
            }
            else
            {
                changeState(STATE_PIR_LOW);
            }
            break;


        /* =================================================
         * PIR HIGH
         * ================================================= */

        case STATE_PIR_HIGH:

            /*
             * PIR đang HIGH.
             *
             * Nếu HIGH liên tục 10 giây:
             *
             * -> ESP32 Deep Sleep
             * -> Wake khi PIR LOW
             */

            if (pirValue == HIGH)
            {
                if (millis() - stateStartTime
                    >= PIR_STABLE_TIME)
                {
                    Serial.println();
                    Serial.println(
                        "[FSM] PIR HIGH 10s"
                    );

                    Serial.println(
                        "[FSM] Sleep -> Wake khi PIR LOW"
                    );

                    changeState(STATE_SLEEP);
                }
            }
            else
            {
                /*
                 * PIR chuyển LOW trước khi đủ 10 giây.
                 *
                 * Reset timer.
                 */

                Serial.println(
                    "[FSM] PIR HIGH -> LOW"
                );

                changeState(STATE_PIR_LOW);
            }

            break;


        /* =================================================
         * PIR LOW
         * ================================================= */

        case STATE_PIR_LOW:

            /*
             * PIR đang LOW.
             *
             * Nếu LOW liên tục 10 giây:
             *
             * -> ESP32 Deep Sleep
             * -> Wake khi PIR HIGH
             */

            if (pirValue == LOW)
            {
                if (millis() - stateStartTime
                    >= PIR_STABLE_TIME)
                {
                    Serial.println();
                    Serial.println(
                        "[FSM] PIR LOW 10s"
                    );

                    Serial.println(
                        "[FSM] Sleep -> Wake khi PIR HIGH"
                    );

                    changeState(STATE_SLEEP);
                }
            }
            else
            {
                /*
                 * PIR chuyển HIGH trước khi đủ 10 giây.
                 *
                 * Reset timer.
                 */

                Serial.println(
                    "[FSM] PIR LOW -> HIGH"
                );

                changeState(STATE_PIR_HIGH);
            }

            break;


        /* =================================================
         * OCCUPIED
         * ================================================= */

        case STATE_OCCUPIED:

            /*
             * Sẽ dùng sau khi tích hợp LD2410C.
             */

            processLD2410();

            if (!radarPresence)
            {
                changeState(STATE_NO_PRESENCE);
            }

            break;


        /* =================================================
         * NO PRESENCE
         * ================================================= */

        case STATE_NO_PRESENCE:

            /*
             * Sẽ dùng sau khi tích hợp LD2410C.
             */

            processLD2410();

            if (radarPresence)
            {
                changeState(STATE_OCCUPIED);
            }

            break;


        /* =================================================
         * SLEEP
         * ================================================= */

        case STATE_SLEEP:

            enterDeepSleep();

            break;
    }
}


/* =========================================================
 * CHANGE STATE
 * ========================================================= */

void changeState(RoomState newState)
{
    currentState = newState;

    /*
     * Mỗi lần chuyển trạng thái,
     * bắt đầu lại bộ đếm thời gian.
     */

    stateStartTime = millis();


    /*
     * In trạng thái để debug
     */

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


/* =========================================================
 * DEEP SLEEP
 * ========================================================= */

void enterDeepSleep()
{
    int pirValue = digitalRead(PIR_PIN);


    /*
     * PIR đang HIGH
     * -> Sleep
     * -> Wake khi PIR LOW
     */

    if (pirValue == HIGH)
    {
        Serial.println(
            "[SLEEP] PIR = HIGH"
        );

        Serial.println(
            "[SLEEP] Wake-up level = LOW"
        );

        esp_sleep_enable_ext0_wakeup(
            (gpio_num_t)PIR_PIN,
            0
        );
    }


    /*
     * PIR đang LOW
     * -> Sleep
     * -> Wake khi PIR HIGH
     */

    else
    {
        Serial.println(
            "[SLEEP] PIR = LOW"
        );

        Serial.println(
            "[SLEEP] Wake-up level = HIGH"
        );

        esp_sleep_enable_ext0_wakeup(
            (gpio_num_t)PIR_PIN,
            1
        );
    }


    Serial.println(
        "[SLEEP] ESP32 -> DEEP SLEEP"
    );

    Serial.flush();


    /*
     * Bắt đầu Deep Sleep
     */

    esp_deep_sleep_start();
}


/* =========================================================
 * WAKEUP REASON
 * ========================================================= */

void printWakeupReason()
{
    esp_sleep_wakeup_cause_t reason;

    reason = esp_sleep_get_wakeup_cause();


    switch (reason)
    {
        case ESP_SLEEP_WAKEUP_EXT0:

            Serial.println(
                "[WAKEUP] Wake-up bang PIR / EXT0"
            );

            break;


        default:

            Serial.println(
                "[WAKEUP] Khoi dong binh thuong"
            );

            break;
    }
}


/* =========================================================
 * LD2410 PROCESS
 * ========================================================= */

void processLD2410()
{
    if (readLD2410())
    {
        Serial.print(
            "[LD2410] Presence = "
        );

        Serial.println(
            radarPresence ? "YES" : "NO"
        );
    }
}


/* =========================================================
 * LD2410 UART
 * ========================================================= */

bool readLD2410()
{
    /*
     * CHƯA PHẢI PARSER HOÀN CHỈNH.
     *
     * Sẽ thay bằng parser frame UART
     * thực tế của LD2410C.
     */

    return false;
}