#include <stdint.h> 
#define BT_CHANNELS 8

void processTrainerByte(uint8_t data);
void bleReconnectDevice();
extern uint16_t BtChannelsIn[BT_CHANNELS];

enum { STATE_DATA_IDLE, STATE_DATA_START, STATE_DATA_XOR, STATE_DATA_IN_FRAME };

#define bluetooth_LINE_LENGTH 32
#define bluetooth_PACKET_SIZE 14
