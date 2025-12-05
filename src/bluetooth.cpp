#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>
#include <map>
#include <opentxbt.h>
#include <ppmout.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "led.h"


#define SCAN_TIME 3          
#define RECONNECT_INTERVAL 2 
#define TARGET_DEVICE_NAME "Hello" 
#define CONNECT_TIMEOUT 5   

#define BT_CHANNELS 8
uint16_t chanoverrides = 0xFFFF; 


// BLE相关全局变量
BLEScan* pBLEScan = nullptr;
BLEClient* pBLEClient = nullptr;
BLERemoteService* pBLETargetService = nullptr;
BLERemoteCharacteristic* pBLETargetCharacteristic = nullptr; 
BLEAddress TARGET_DEVICE_ADDR("00:00:00:00:00:00"); 


// 连接状态标志 - 在FreeRTOS环境中需要线程安全的状态变量
extern bool isConnected; // 在头文件中声明为extern
bool isConnected = false; // 在源文件中定义
bool isDeviceFound = false;                          
bool isServiceFound = false;
bool isCharacteristicFound = false;
bool isNotifyEnabled = false; 
bool scanStarted = false; // 扫描状态标志
unsigned long stateTimestamp = 0; // 状态时间戳
uint16_t  notifyDataSerialOut= 0;

// 状态机相关变量
enum BLEState {
    BLE_STATE_INIT,
    BLE_STATE_SCANNING,
    BLE_STATE_CONNECTING,
    BLE_STATE_FINDING_SERVICE,
    BLE_STATE_FINDING_CHARACTERISTIC,
    BLE_STATE_ENABLING_NOTIFY,
    BLE_STATE_CONNECTED,
    BLE_STATE_RECONNECTING
};

BLEState currentState = BLE_STATE_INIT;


const std::string TARGET_SERVICE_UUID = "0000fff0-0000-1000-8000-00805f9b34fb";
const std::string TARGET_CHARACTERISTIC_UUID = "0000fff6-0000-1000-8000-00805f9b34fb"; 
static uint16_t chan_vals[BT_CHANNELS];

class HTBLEClientCallbacks : public BLEClientCallbacks {
public:
    void onConnect(BLEClient* pClient) override {
        Serial.println("[BLE Callback]  Device connected!");
        isConnected = true;
        currentState = BLE_STATE_FINDING_SERVICE;
        stateTimestamp = millis();
        
        // 连接成功后，控制LED常亮
        ledSetState(LED_STATE_ON);
    }

    void onDisconnect(BLEClient* pClient) override {
        Serial.println("\n[BLE Callback]   Device disconnected!");
        isConnected = false;
        isServiceFound = false;
        isCharacteristicFound = false;
        isNotifyEnabled = false;
        pBLETargetService = nullptr;
        pBLETargetCharacteristic = nullptr;
        currentState = BLE_STATE_RECONNECTING;
        stateTimestamp = millis();
        scanStarted = false;
        
        // 断开连接后，控制LED闪烁（搜索时）
        ledSetState(LED_STATE_BLINKING_FAST);
    }
};

// -------------------------- Notify Callback (Receive Raw Data) --------------------------
void notifyCallback(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {

    for (int i = 0; i < length; i++) {
        processTrainerByte(pData[i]);
    }
    // 同步通道数据
    for (int i = 0; i < BT_CHANNELS; i++) {
        chan_vals[i] = BtChannelsIn[i];
    }
    // 输出通道数据
    setPPMChannelValues(chan_vals);
    notifyDataSerialOut++;
    if(notifyDataSerialOut>=1000){
    notifyDataSerialOut=0;
    Serial.printf(" Received raw data (length: %d bytes): ", length);
    for (size_t i = 0; i < BT_CHANNELS; i++) {
        Serial.printf("%d", chan_vals[i]);
    }
    Serial.println(); 
    }

}

void btChannelsDecoded(uint16_t *channels) {
    for (int i = 0; i < BT_CHANNELS; i++) {
        chan_vals[i] = channels[i];
    }
}

uint16_t btGetChannel(int channel)
{
    if (channel >= 0 && channel < BT_CHANNELS && isConnected && ((1 << channel) & chanoverrides)) {
        return chan_vals[channel];
    }
    return 0;
}

class ScanCallback : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        std::string deviceName = advertisedDevice.getName();
        BLEAddress deviceAddr = advertisedDevice.getAddress();
        Serial.printf("[Scan] Discovered device: Name='%s' MAC='%s'\n", deviceName.c_str(), deviceAddr.toString().c_str());

        if (!deviceName.empty() && deviceName == TARGET_DEVICE_NAME) {
            isDeviceFound = true;
            TARGET_DEVICE_ADDR = deviceAddr;
            pBLEScan->stop(); 
            Serial.printf("[Scan]   Target device found: Name='%s' MAC='%s'\n", deviceName.c_str(), TARGET_DEVICE_ADDR.toString().c_str());
            currentState = BLE_STATE_CONNECTING;
            stateTimestamp = millis();
        }
    }
};


void bleHostInit() {
    Serial.println("[BLE Init] Starting BLE Host mode...");
    BLEDevice::init("ESP32_BLE_Host"); 

    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new ScanCallback());
    pBLEScan->setActiveScan(true);  
    pBLEScan->setInterval(100);     
    pBLEScan->setWindow(99);        
    pBLEScan->clearResults();        

    pBLEClient = BLEDevice::createClient();
    pBLEClient->setClientCallbacks(new HTBLEClientCallbacks()); // 注册连接回调

    Serial.println("[BLE Init]   Initialization completed!");
    currentState = BLE_STATE_SCANNING;
    stateTimestamp = millis();
}


// 处理BLE初始化状态
void bleInitState() {
    // 已在bleHostInit中初始化完成，进入扫描状态
    currentState = BLE_STATE_SCANNING;
    stateTimestamp = millis();
}

// 处理BLE扫描状态
void bleScanningState(unsigned long currentTime) {
    if (!scanStarted) {
        scanStarted = true;
        isDeviceFound = false;
        TARGET_DEVICE_ADDR = BLEAddress("00:00:00:00:00:00");
        Serial.printf("[Scan] Scanning for '%s' (duration: %d seconds)...\n", TARGET_DEVICE_NAME, SCAN_TIME);
        pBLEScan->clearResults();
        pBLEScan->start(SCAN_TIME, false);
        stateTimestamp = currentTime;
    } else if (currentTime - stateTimestamp > (SCAN_TIME * 1000)) {
        // 扫描超时，重新开始扫描
        scanStarted = false;
        pBLEScan->stop();
        pBLEScan->clearResults();
        Serial.println("[Scan] Scan timeout, restarting...");
    }
}

// 处理BLE连接状态
void bleConnectingState(unsigned long currentTime) {
    if (TARGET_DEVICE_ADDR.toString() == "00:00:00:00:00:00") {
        Serial.println("[Connect] Error: Target device not found!");
        currentState = BLE_STATE_SCANNING;
        scanStarted = false;
        return;
    }
    
    if (pBLEClient->isConnected()) {
        pBLEClient->disconnect();
        Serial.println("[Connect] Disconnected existing connection");
    }
    
    Serial.printf("[Connect] Connecting to '%s' (MAC: %s)...\n", TARGET_DEVICE_NAME, TARGET_DEVICE_ADDR.toString().c_str());
    bool connectOk = pBLEClient->connect(TARGET_DEVICE_ADDR, BLE_ADDR_TYPE_RANDOM);
    
    if (!connectOk) {
        Serial.println("[Connect] Connection failed!");
        currentState = BLE_STATE_RECONNECTING;
        stateTimestamp = currentTime;
    }
    // 连接成功会通过回调处理
}

// 处理BLE查找服务状态
void bleFindingServiceState(unsigned long currentTime) {
    if (!isConnected) {
        currentState = BLE_STATE_RECONNECTING;
        stateTimestamp = currentTime;
        return;
    }
    
    Serial.println("[Service] Exploring GATT services...");
    BLEUUID serviceUUID(TARGET_SERVICE_UUID);
    pBLETargetService = pBLEClient->getService(serviceUUID);
    
    if (pBLETargetService != nullptr) {
        isServiceFound = true;
        Serial.printf("[Service] Found target service: %s\n", TARGET_SERVICE_UUID.c_str());
        currentState = BLE_STATE_FINDING_CHARACTERISTIC;
        stateTimestamp = currentTime;
    } else {
        isServiceFound = false;
        Serial.printf("[Service] Target service not found: %s\n", TARGET_SERVICE_UUID.c_str());
        currentState = BLE_STATE_RECONNECTING;
        stateTimestamp = currentTime;
    }
}

// 处理BLE查找特征状态
void bleFindingCharacteristicState(unsigned long currentTime) {
    if (!isServiceFound || pBLETargetService == nullptr) {
        Serial.println("[Characteristic] Error: Target service not found!");
        currentState = BLE_STATE_RECONNECTING;
        stateTimestamp = currentTime;
        return;
    }
    
    Serial.println("[Characteristic] Exploring target service characteristics...");
    pBLETargetCharacteristic = pBLETargetService->getCharacteristic(TARGET_CHARACTERISTIC_UUID);
    
    if (pBLETargetCharacteristic != nullptr) {
        isCharacteristicFound = true;
        Serial.printf("[Characteristic] Found target characteristic: %s\n", TARGET_CHARACTERISTIC_UUID.c_str());
        currentState = BLE_STATE_ENABLING_NOTIFY;
        stateTimestamp = currentTime;
    } else {
        isCharacteristicFound = false;
        Serial.printf("[Characteristic] Target characteristic not found: %s\n", TARGET_CHARACTERISTIC_UUID.c_str());
        currentState = BLE_STATE_RECONNECTING;
        stateTimestamp = currentTime;
    }
}

// 处理BLE启用通知状态
void bleEnablingNotifyState(unsigned long currentTime) {
    if (!isCharacteristicFound || pBLETargetCharacteristic == nullptr) {
        Serial.println("[Notify] Error: Target characteristic not found!");
        currentState = BLE_STATE_RECONNECTING;
        stateTimestamp = currentTime;
        return;
    }
    
    if (!pBLETargetCharacteristic->canNotify()) {
        Serial.println("[Notify] Characteristic does not support notify!");
        currentState = BLE_STATE_RECONNECTING;
        stateTimestamp = currentTime;
        return;
    }
    
    pBLETargetCharacteristic->registerForNotify(notifyCallback);
    isNotifyEnabled = true;
    Serial.println("[Notify] Notify mode enabled! Waiting for data...");
    currentState = BLE_STATE_CONNECTED;
    stateTimestamp = currentTime;
}

// 处理BLE已连接状态
void bleConnectedState(unsigned long currentTime) {
    // 连接已建立，数据通过notify回调接收
    // 定期检查连接状态
    if (currentTime - stateTimestamp > 5000) { // 每5秒检查一次
        if (pBLEClient && !pBLEClient->isConnected()) {
            Serial.println("[Connection Check] Connection lost!");
            currentState = BLE_STATE_RECONNECTING;
            stateTimestamp = currentTime;
        } else {
            stateTimestamp = currentTime;
        }
    }
}

// 处理BLE重连状态
void bleReconnectingState(unsigned long currentTime) {
    // 重连间隔
    if (currentTime - stateTimestamp > (RECONNECT_INTERVAL * 1000)) {
        Serial.println("[Reconnect] Attempting to reconnect...");
        // 重置状态
        isDeviceFound = false;
        isServiceFound = false;
        isCharacteristicFound = false;
        isNotifyEnabled = false;
        pBLETargetService = nullptr;
        pBLETargetCharacteristic = nullptr;
        
        currentState = BLE_STATE_SCANNING;
        scanStarted = false;
    }
}

// BLE状态机处理函数
void bleProcessStateMachine() {
    unsigned long currentTime = millis();
    
    switch (currentState) {
        case BLE_STATE_INIT:
            bleInitState();
            break;
        case BLE_STATE_SCANNING:
            bleScanningState(currentTime);
            break;
        case BLE_STATE_CONNECTING:
            bleConnectingState(currentTime);
            break;
        case BLE_STATE_FINDING_SERVICE:
            bleFindingServiceState(currentTime);
            break;
        case BLE_STATE_FINDING_CHARACTERISTIC:
            bleFindingCharacteristicState(currentTime);
            break;
        case BLE_STATE_ENABLING_NOTIFY:
            bleEnablingNotifyState(currentTime);
            break;
        case BLE_STATE_CONNECTED:
            bleConnectedState(currentTime);
            break;
        case BLE_STATE_RECONNECTING:
            bleReconnectingState(currentTime);
            break;
    }
}

void bleDisconnectDevice() {
    if (pBLEClient && pBLEClient->isConnected()) {
        pBLEClient->disconnect();
        isConnected = false;
        Serial.println("[Disconnect] Disconnected successfully!");
    } else {
        Serial.println("[Disconnect] Not connected to any device");
    }
}

// 从opentxbt.h声明的函数，需要在这里实现
void bleReconnectDevice() {
    Serial.println("[Reconnect] Starting reconnection process...");
    currentState = BLE_STATE_RECONNECTING;
    stateTimestamp = millis();
}


void bleStart() {
    // 开始蓝牙操作前，设置LED为搜索状态（快速闪烁）
    ledSetState(LED_STATE_BLINKING_FAST);
    
    currentState = BLE_STATE_INIT;
    stateTimestamp = millis();
}


void bleinit() {
    Serial.println("Initializing BLE...");
    bleHostInit();
    isConnected = false;
    
    // 初始化后设置LED为搜索状态（快速闪烁）
    ledSetState(LED_STATE_BLINKING_FAST);
    
    Serial.println("BLE initialized");
}

// 蓝牙任务函数
void bluetoothTask(void *pvParameters) {
    // 初始化蓝牙
    bleinit();
    
    // 蓝牙任务主循环
    while (1) {
        // 运行蓝牙状态机
        bleProcessStateMachine();
        // 让出CPU，允许其他任务运行
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}