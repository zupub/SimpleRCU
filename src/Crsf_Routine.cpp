#include <CrsfSerial.h>
#include <median.h>
#include <ESP32Servo.h>
#include "Crsf_Routine.h"

// Ref: https://github.com/crsf-wg/crsf/wiki/CRSF_FRAMETYPE_RC_CHANNELS_PACKED

// 硬件串口配置
HardwareSerial CrsfSerialStream(2);
static CrsfSerial crsf(CrsfSerialStream);

QueueHandle_t crsfQueue;

// Configuration
// Map input CRSF channels (1-based, up to 16 for CRSF, 12 for ELRS) to outputs 1-8
// use a negative number to invert the signal (i.e. +100% becomes -100%)
constexpr int OUTPUT_MAP[CHANNEL_NUM] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
volatile uint16_t rcChannels[CHANNEL_NUM]; // 存储解码后的PWM值（1000-2000us）

Servo servo1, servo2;
int servo1Pin = PWM_PIN_CH1;
int servo2Pin = PWM_PIN_CH2;
int minUs = SERVO_US_MIN;
int maxUs = SERVO_US_MAX;
int freq = SERVO_PWM_FREQ;

static void packetChannels()
{
    crsfData_t crsfData;

    for (unsigned int out = 0; out < CHANNEL_NUM; ++out)
    {
        const int chInput = OUTPUT_MAP[out];
        int usOutput;
        if (chInput > 0)
        {
            usOutput = crsf.getChannel(chInput);
            rcChannels[out] = usOutput;
            crsfData.pwmChannels[out] = usOutput;
        }
        else
        {
            // if chInput is negative, invert the channel output
            usOutput = crsf.getChannel(-chInput);
            // (1500 - usOutput) + 1500
            usOutput = 3000 - usOutput;
            crsfData.pwmChannels[out] = usOutput;
        }
    }

    // 刷新舵机位置
    updateServo();

    // 发送到显示队列
    if (xQueueSend(crsfQueue, &crsfData, 0) != pdTRUE)
    {
        Serial.println("CRSF Queue Full!");
    }

#if 0
    for (unsigned int ch = 1; ch <= 4; ++ch)
    {
        Serial.write(ch < 10 ? '0' + ch : 'A' + ch - 10);
        Serial.write('=');
        Serial.print(crsf.getChannel(ch), DEC);
        Serial.write(' ');
    }
    Serial.println();
#endif
}

static void packetLinkStatistics(crsfLinkStatistics_t *link)
{
    Serial.print(link->uplink_RSSI_1, DEC);
    Serial.println("dBm");
}

static void crsfLinkUp()
{
    Serial.println("crsfLinkUp");
}

static void crsfLinkDown()
{
    Serial.println("crsfLinkDown");
    // outputFailsafeValues();
}

static void crsfOobData(uint8_t b)
{
    // A shifty byte is usually just log messages from ELRS
    Serial.write(b);
}

static void setupCrsf()
{
    CrsfSerialStream.begin(CRSF_BUADRATE, SERIAL_8N1, CRSF_RX_PIN, CRSF_TX_PIN);
    // crsf.onLinkUp = &crsfLinkUp;
    // crsf.onLinkDown = &crsfLinkDown;
    // crsf.onOobData = &crsfOobData;
    crsf.onPacketChannels = &packetChannels;
    // crsf.onPacketLinkStatistics = &packetLinkStatistics;
    crsf.begin();
}

void crsfTask(void *pv)
{
    while (1)
    {
        crsf.loop();
        vTaskDelay(2 / portTICK_PERIOD_MS);
    }
}

void setupServo()
{
    servo1.setPeriodHertz(freq);
    servo2.setPeriodHertz(freq);
    servo1.attach(servo1Pin, minUs, maxUs);
    servo2.attach(servo2Pin, minUs, maxUs);
}

void updateServo()
{
    servo1.writeMicroseconds(rcChannels[3]);
    servo2.writeMicroseconds(rcChannels[1]);
}

void Crsf_setup()
{
    setupCrsf();
    setupServo();
    crsfQueue = xQueueCreate(3, sizeof(crsfData_t));
    xTaskCreatePinnedToCore(crsfTask, "crsfTask", 2048, NULL, 4, NULL, 1);
}
