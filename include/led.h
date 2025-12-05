#include <Arduino.h>

#ifndef  LED_H  // 头文件保护宏：防止重复包含
#define LED_H

// LED引脚定义
#define LED_PIN 8

// LED状态枚举
typedef enum {
    LED_STATE_OFF,
    LED_STATE_ON,
    LED_STATE_BLINKING_FAST,  // 快速闪烁（用于搜索）
    LED_STATE_BLINKING_SLOW   // 慢速闪烁（1秒一次）
} LEDState;

// 初始化LED
void ledInit();

// 设置LED状态
void ledSetState(LEDState state);

// LED状态更新函数（需要在主循环中调用）
void ledUpdate();

// 根据蓝牙连接状态更新LED
void ledUpdateByBleState(bool isConnected);

#endif