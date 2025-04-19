#ifndef CRSF_ROUTINE_H
#define CRSF_ROUTINE_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <ESP32Servo.h>

#define CRSF_BUADRATE 420800

// CRSF接收机引脚定义
#define CRSF_RX_PIN 17
#define CRSF_TX_PIN 16

// PWM输出引脚定义
#define PWM_PIN_CH1 5
#define PWM_PIN_CH2 26

#define SERVO_US_MIN 1140
#define SERVO_US_MAX 1835
#define SERVO_PWM_FREQ 59

#define CHANNEL_NUM 16

typedef struct
{
    uint16_t pwmChannels[CHANNEL_NUM];
} crsfData_t;

void Crsf_setup();
void setupServo();
void updateServo();

#endif
