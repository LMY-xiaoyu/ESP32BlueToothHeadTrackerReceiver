#include <stdint.h> 
#define BT_CHANNELS 8
void btChannelsDecoded(uint16_t *channels);
uint16_t btGetChannel(int channel);
void bleStart();
void bleinit();

// 状态机处理函数声明
void bleProcessStateMachine();

// 蓝牙连接状态和相关变量的外部声明
extern bool isConnected;
extern bool isDeviceFound;
extern bool isServiceFound;
extern bool isCharacteristicFound;
extern bool isNotifyEnabled;
extern bool scanStarted;
extern unsigned long stateTimestamp;

extern void bluetoothTask(void *pvParameters);
extern void ledTask(void *pvParameters);