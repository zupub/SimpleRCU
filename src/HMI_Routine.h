#ifndef HMI_ROUTINE_H
#define HMI_ROUTINE_H

#define BUTTON_A_PIN 39
#define BUTTON_B_PIN 38
#define BUTTON_C_PIN 37
#define DEBOUNCE_MS 10

#define TFT_BL_PIN 32
#define BL_PWM_CHANNEL 2
#define BL_PWM_DUTY 128

void Disp_setup();
void Boot_screen();

#endif
