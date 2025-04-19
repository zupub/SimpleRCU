#include <TFT_eSPI.h>
#include "HMI_Routine.h"
#include "Crsf_Routine.h"
#include "driver/Button.h"
#include "assets/NotoSansBold15.h"
#include "assets/NotoSansBold36.h"

TFT_eSPI tft = TFT_eSPI();                  /* TFT instance */
TFT_eSprite mainCanvas = TFT_eSprite(&tft); // 主显示画布

Button BtnA = Button(BUTTON_A_PIN, true, DEBOUNCE_MS);
Button BtnB = Button(BUTTON_B_PIN, true, DEBOUNCE_MS);
Button BtnC = Button(BUTTON_C_PIN, true, DEBOUNCE_MS);

extern QueueHandle_t crsfQueue;
crsfData_t currentData;

uint8_t page_index = 0;

// 全局PWM数据 (示例值)
int pwmData[CHANNEL_NUM] = {
    1500, 1500, // 左摇杆X,Y (通道0,1)
    1500, 1500, // 右摇杆X,Y (通道2,3)
    1500, 1500, // SA,SB (通道4,5)
    1500, 1500, // SC,SD (通道6,7)
    1500, 1500  // SE,SF (通道8,9)
};

uint8_t getButton(void)
{
    BtnA.read();
    BtnB.read();
    BtnC.read();

    if (BtnA.wasPressed())
    {
        page_index = page_index <= 0 ? 0 : --page_index;
        Serial.printf("page_index: %d\r\n", page_index);
    }

    if (BtnB.wasPressed())
    {
        page_index = page_index >= 3 ? 3 : ++page_index;
        Serial.printf("page_index: %d\r\n", page_index);
    }

    if (BtnC.wasReleasefor(100))
    {
        Serial.println("BtnC.wasReleasefor(100)");
    }

    if (BtnC.pressedFor(1000))
    {
        while (BtnC.read())
            ;
        Serial.println("BtnC.pressedFor(1000)");
    }

    return page_index;
}

void initDisplay()
{
    tft.setRotation(1); // 根据实际安装方向调整

    // 主画布使用16色压缩模式
    mainCanvas.setColorDepth(8);
    mainCanvas.createSprite(tft.width(), tft.height());

    // 预加载字体
    mainCanvas.setTextSize(1);
    mainCanvas.setTextColor(TFT_WHITE, TFT_BLACK);
}

/*
+--------------------------------+
|       CRSF Channel             |
| CH1 [============      ] 1720μs|
| CH2 [==================] 2000μs|
| CH3 [                  ] 1000μs|
| CH4 [=====             ] 1320μs|
| CH5 [===========       ] 1880μs|
| CH6 [==                ] 1100μs|
+--------------------------------+
*/
void drawChannelBar(int ch, int x, int y, int w, int h, uint16_t color)
{
    float percent = 0;
    int fillWidth = 0;
    // 计算填充比例（PWM 1000-2000 → 0-100%）
    // float percent = rcChannels[ch] / 2120.0;
    if (xQueueReceive(crsfQueue, &currentData, pdMS_TO_TICKS(20)) == pdPASS)
    {
        percent = currentData.pwmChannels[ch] / 2120.0;
        fillWidth = constrain(w * percent, 0, w);
    }
    // 绘制背景框
    mainCanvas.drawRect(x, y, w, h, TFT_WHITE);
    // 填充进度
    mainCanvas.fillRect(x + 1, y + 1, fillWidth - 2, h - 1, color);

    // 添加标签与数值
    mainCanvas.setTextColor(TFT_WHITE, TFT_BLACK);
    mainCanvas.setTextSize(2);
    mainCanvas.drawString("CH" + String(ch + 1), 10, y + 5); // 左侧标签
    mainCanvas.drawString(String(pwmData[ch]), 270, y + 5);  // 右侧数值
}

void _page_Crsf()
{
    mainCanvas.fillSprite(TFT_BLACK); // 清空画布

    const int barWidth = 200; // 进度条宽度
    const int barHeight = 20; // 进度条高度
    const int spacing = 24;   // 垂直间距

    // 绘制6个通道的进度条（纵向居中布局）
    int startX = 55;
    int startY = 35;
    for (int i = 0; i < 8; i++)
    {
        drawChannelBar(i, startX, startY + i * spacing, barWidth, barHeight, TFT_GREEN);
    }

    // 添加标题
    mainCanvas.setTextColor(TFT_WHITE, TFT_BLACK);
    mainCanvas.setTextSize(3);
    mainCanvas.drawString("CRSF Channels", 30, 8);
    mainCanvas.pushSprite(0, 0); // 推送画布到屏幕
}

// ---------------------------------------------------------------------
// 硬件参数
#define SCREEN_W TFT_HEIGHT
#define SCREEN_H TFT_WIDTH
#define PWM_MIN 1000 // PWM最小值
#define PWM_MAX 2000 // PWM最大值

// 摇杆绘制函数
void drawStick(int x, int y, int pwmX, int pwmY)
{
    // 绘制底座
    mainCanvas.drawRoundRect(x, y, 80, 80, 8, TFT_DARKGREY);

    // 坐标映射
    int posX = map(constrain(pwmX, PWM_MIN, PWM_MAX), PWM_MIN, PWM_MAX, x, x + 80);
    int posY = map(constrain(pwmY, PWM_MIN, PWM_MAX), PWM_MIN, PWM_MAX, y, y + 80);

    // 绘制摇杆头
    mainCanvas.fillCircle(posX, y + 210 - posY, 12, TFT_GREEN);

    mainCanvas.drawLine(x + 40 - 5, y + 40, x + 40 + 5, y + 40, TFT_DARKGREY);
    mainCanvas.drawLine(x + 40, y + 40 - 5, x + 40, y + 40 + 5, TFT_DARKGREY);
}

// 获取开关状态符号
String getSwitchState(int pwm)
{
    if (pwm < 1200)
        return ": ^"; // 上档
    else if (pwm > 1800)
        return ": v"; // 下档
    else
        return ": -"; // 中档
}

// 开关状态绘制
void drawSwitches()
{
    // 开关位置参数
    const int baseX[] = {5, 55, 105, 155, 205, 255};
    const int baseY = 80;
    const int yStep = 35;

    // 绘制所有开关状态
    for (int i = 0; i < 6; i++)
    {
        int channel = i + 4; // 通道4-9对应SA-SF
        String state = getSwitchState(pwmData[channel]);

        int xPos = baseX[i] + 10;
        int yPos = baseY;
        String label = "S" + String(char('A' + i)) + state;

        mainCanvas.loadFont(NotoSansBold15);
        mainCanvas.drawString(label, xPos, yPos);
        mainCanvas.unloadFont();
    }
}

// 界面绘制主函数
void _page_Joystick()
{
    mainCanvas.fillSprite(TFT_BLACK);
    mainCanvas.setTextColor(TFT_WHITE, TFT_BLACK);

    // 顶部状态栏
    mainCanvas.loadFont(NotoSansBold36);
    mainCanvas.drawString("Joystick", 90, 10);
    mainCanvas.unloadFont();

    // 绘制摇杆系统
    drawStick(50, 130, pwmData[3], pwmData[2]);  // 左摇杆
    drawStick(190, 130, pwmData[0], pwmData[1]); // 右摇杆

    // 绘制开关状态
    drawSwitches();

    mainCanvas.pushSprite(0, 0);
}

void updatePWMData()
{
    if (xQueueReceive(crsfQueue, &currentData, pdMS_TO_TICKS(20)) == pdPASS)
    {
        for (int i = 0; i < CHANNEL_NUM; i++)
        {
            pwmData[i] = currentData.pwmChannels[i];
        }
    }

#if 0
    // PWM数据更新模拟，左右摇杆随机微调
    for (int i = 0; i < 4; i++)
    {
        pwmData[i] = constrain(pwmData[i] + random(-5, 5), PWM_MIN, PWM_MAX);
    }
    // 示例：随机切换开关状态
    static unsigned long lastSwitch = 0;
    if (millis() - lastSwitch > 1000)
    {
        pwmData[4 + random(6)] = random(3) ? 1500 : (random(2) ? PWM_MIN : PWM_MAX);
        lastSwitch = millis();
    }
#endif
}

// ---------------------------------------------------------------------
void displayTask(void *pv)
{
    initDisplay();

    while (1)
    {
        getButton();
        updatePWMData();

        if (page_index == 0)
        {
            _page_Crsf();
        }
        else if (page_index == 1)
        {
            _page_Joystick();
        }

        vTaskDelay(2 / portTICK_PERIOD_MS);
    }
}

void Disp_setup()
{
    xTaskCreatePinnedToCore(displayTask, "Display", 4096, NULL, 1, NULL, 0);
}

TFT_eSprite img = TFT_eSprite(&tft); // Create Sprite object "img" with pointer to "tft" object

// #########################################################################
// Create sprite, plot graphics in it, plot to screen, then delete sprite
// #########################################################################
void drawStar(int x, int y, int star_color)
{
    // Create an 8-bit sprite 70x 80 pixels (uses 5600 bytes of RAM)
    img.setColorDepth(8);
    img.createSprite(70, 80);

    // Fill Sprite with a "transparent" colour
    // TFT_TRANSPARENT is already defined for convenience
    // We could also fill with any colour as "transparent" and later specify that
    // same colour when we push the Sprite onto the screen.
    img.fillSprite(TFT_TRANSPARENT);

    // Draw 2 triangles to create a filled in star
    img.fillTriangle(35, 0, 0, 59, 69, 59, star_color);
    img.fillTriangle(35, 79, 0, 20, 69, 20, star_color);

    // Punch a star shaped hole in the middle with a smaller transparent star
    img.fillTriangle(35, 7, 6, 56, 63, 56, TFT_TRANSPARENT);
    img.fillTriangle(35, 73, 6, 24, 63, 24, TFT_TRANSPARENT);

    // Push sprite to TFT screen CGRAM at coordinate x,y (top left corner)
    // Specify what colour is to be treated as transparent.
    img.pushSprite(x, y, TFT_TRANSPARENT);

    // Delete it to free memory
    img.deleteSprite();
}

// #########################################################################
// Draw a number in a rounded rectangle with some transparent pixels
// #########################################################################

// Size of sprite
#define IWIDTH 80
#define IHEIGHT 35

void numberBox(int x, int y, float num)
{
    // Create a 8-bit sprite 80 pixels wide, 35 high (2800 bytes of RAM needed)
    img.setColorDepth(8);
    img.createSprite(IWIDTH, IHEIGHT);

    // Fill it with black (this will be the transparent colour this time)
    img.fillSprite(TFT_BLACK);

    // Draw a background for the numbers
    img.fillRoundRect(0, 0, 80, 35, 15, TFT_RED);
    img.drawRoundRect(0, 0, 80, 35, 15, TFT_WHITE);

    // Set the font parameters
    img.setTextSize(1);          // Font size scaling is x1
    img.setTextColor(TFT_WHITE); // White text, no background colour

    // Set text coordinate datum to middle right
    img.setTextDatum(MR_DATUM);

    // Draw the number to 3 decimal places at 70,20 in font 4
    img.drawFloat(num, 3, 70, 20, 4);

    // Push sprite to TFT screen CGRAM at coordinate x,y (top left corner)
    // All black pixels will not be drawn hence will show as "transparent"
    img.pushSprite(x, y, TFT_BLACK);

    // Delete sprite to free up the RAM
    img.deleteSprite();
}

void Boot_screen()
{
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_NAVY);

    ledcSetup(BL_PWM_CHANNEL, 200, 8);
    ledcAttachPin(TFT_BL_PIN, BL_PWM_CHANNEL);
    ledcWrite(BL_PWM_CHANNEL, BL_PWM_DUTY);

    // Draw 10 sprites containing a "transparent" colour
    for (int i = 0; i < 10; i++)
    {
        int x = random(240 - 70);
        int y = random(320 - 80);
        int c = random(0x10000); // Random colour
        drawStar(x, y, c);
    }

    delay(1000);
}
