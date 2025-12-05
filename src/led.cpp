#include "led.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 全局变量
static LEDState currentState = LED_STATE_OFF;
static unsigned long lastBlinkTime = 0;
static bool isBlinkOn = false;
static const unsigned long BLINK_INTERVAL_FAST = 200;  // 快速闪烁间隔，单位毫秒
static const unsigned long BLINK_INTERVAL_SLOW = 1000; // 慢速闪烁间隔，单位毫秒（1秒）

/**
 * 初始化LED
 */
void ledInit() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // 初始状态为关闭（低电平亮，所以高电平是关闭）
    currentState = LED_STATE_OFF;
    lastBlinkTime = millis();
}

/**
 * 设置LED状态
 * @param state LED状态（OFF, ON, BLINKING）
 */
void ledSetState(LEDState state) {
    currentState = state;
    
    switch (state) {
        case LED_STATE_OFF:
            digitalWrite(LED_PIN, HIGH); // 关闭LED（低电平亮）
            break;
        case LED_STATE_ON:
            digitalWrite(LED_PIN, LOW); // 打开LED
            break;
        case LED_STATE_BLINKING_FAST:
        case LED_STATE_BLINKING_SLOW:
            // 闪烁状态由ledUpdate处理，重置闪烁计时器
            lastBlinkTime = millis();
            isBlinkOn = false;
            break;
    }
}

/**
 * LED状态更新函数
 * 需要在LED任务中定期调用
 */
void ledUpdate() {
    // 根据不同的闪烁模式使用不同的间隔
    unsigned long blinkInterval = 0;
    bool isBlinkingState = false;
    
    switch (currentState) {
        case LED_STATE_BLINKING_FAST:
            blinkInterval = BLINK_INTERVAL_FAST;
            isBlinkingState = true;
            break;
        case LED_STATE_BLINKING_SLOW:
            blinkInterval = BLINK_INTERVAL_SLOW;
            isBlinkingState = true;
            break;
        default:
            isBlinkingState = false;
            break;
    }
    
    if (isBlinkingState) {
        unsigned long currentTime = millis();

        // 检查是否需要切换闪烁状态
        if (currentTime - lastBlinkTime >= blinkInterval) {
            lastBlinkTime = currentTime;
            isBlinkOn = !isBlinkOn;
            digitalWrite(LED_PIN, isBlinkOn ? LOW : HIGH); // 低电平亮灯
        }
    }
}

/**
 * 根据蓝牙连接状态更新LED
 * @param isConnected 蓝牙是否已连接
 */
// LED任务函数
void ledTask(void *pvParameters) {
    // 初始化LED
    ledInit();
    
    // LED任务主循环
    while (1) {
        // 只更新LED状态，不依赖蓝牙状态
        ledUpdate();
        
        // 让出CPU，允许其他任务运行
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}