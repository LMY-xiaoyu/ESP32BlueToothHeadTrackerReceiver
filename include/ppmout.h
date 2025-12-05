#pragma once
#include <stdint.h>
#include <Arduino.h>
void ppm_init(uint8_t sPin, uint8_t sChannels);
void ppmStartAndstop(bool isStop);
void resetPPMChannels();
void setPPMChannelValues(uint16_t *values);
void setPPMChannels(uint8_t sChannels);
void setPPMFrameLength(uint32_t sFrameLength);
void setPPMSyncLength(uint16_t sPPMSyncLength);