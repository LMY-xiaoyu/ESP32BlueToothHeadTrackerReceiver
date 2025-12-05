#include <Arduino.h>
#include <bluetooth.h>
#include "led.h"
#include "ppmout.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 任务句柄
TaskHandle_t xBluetoothTaskHandle = NULL;
TaskHandle_t xLEDTaskHandle = NULL;
TaskHandle_t xDataSyncTaskHandle = NULL;

// 声明蓝牙连接状态，需要在bluetooth.cpp中实现
extern bool isConnected;


void setup() {
    Serial.begin(115200);
    while (!Serial); // Wait for serial port connection (for ESP32)
    delay(2000);
    
    Serial.println("Starting ESP32 Bluetooth Head Tracker with FreeRTOS tasks...");
    
    // 初始化PPM输出
    ppm_init(6,8);
    // // 创建蓝牙任务
    xTaskCreatePinnedToCore(
        bluetoothTask,           // 任务函数
        "BluetoothTask",        // 任务名称
        4096,                    // 任务堆栈大小
        NULL,                    // 任务参数
        5,                       // 任务优先级
        &xBluetoothTaskHandle,   // 任务句柄
        0                        // CPU核心0
    );
    
    // 创建LED任务
    xTaskCreatePinnedToCore(
        ledTask,                 // 任务函数
        "LEDTask",               // 任务名称
        2048,                    // 任务堆栈大小
        NULL,                    // 任务参数
        4,                       // 任务优先级
        &xLEDTaskHandle,         // 任务句柄
        0                        // CPU核心0
    );
    
    
    Serial.println("All tasks created successfully.");
}

void loop() {
    vTaskDelay(10 / portTICK_PERIOD_MS); // 降低主循环频率
}