#include "ppmout.h"
#include "esp32-hal-timer.h"
#define PPM_FRAME_LENGTH 22500     // ppm帧长度
#define PPM_MAX_FRAME_LENGTH 10000 // 最大ppm帧长度
#define PPM_MIN_FRAME_LENGTH 30000 // 最小ppm帧长度

#define PPM_SYNC_LENGTH 400      // 同步脉冲长度
#define PPM_MAX_SYNC_LENGTH 100  // 最大同步脉冲长度
#define PPM_MIN_SYNC_LENGTH 1000 // 最小同步脉冲长度

#define PPM_CHANNELS 8             // ppm通道数
#define OUTPUT_PIN 6               // ppm输出阵脚
#define DEFAULT_TIMEER_DIVIDER 80  // 时钟分频
#define DEFAULT_CHANNEL_VALUE 1500 // 默认通道值
#define MIN_CHANNEL_VALUE 900      // 最小通道值(微秒)
#define MAX_CHANNEL_VALUE 2100     // 最大通道值(微秒)
#define TARGET_TIMER_CLOCK_MHZ 1   // 目标定时器时钟：1MHz（1微秒/计数）

uint8_t channels = PPM_CHANNELS;
uint16_t channelValue[PPM_CHANNELS] = {DEFAULT_CHANNEL_VALUE, DEFAULT_CHANNEL_VALUE, DEFAULT_CHANNEL_VALUE, DEFAULT_CHANNEL_VALUE, DEFAULT_CHANNEL_VALUE, DEFAULT_CHANNEL_VALUE, DEFAULT_CHANNEL_VALUE, DEFAULT_CHANNEL_VALUE};
uint32_t frameLength = PPM_FRAME_LENGTH;
uint16_t ppmSyncLenth = PPM_SYNC_LENGTH;
uint8_t ppmPin = OUTPUT_PIN;
uint8_t timeDivider = DEFAULT_TIMEER_DIVIDER;
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

enum ppmState_e
{
    PPM_STATE_IDLE,
    PPM_STATE_SYNC,
    PPM_STATE_CHANNEL,
    PPM_STATE_FINALL
};

uint8_t cur_ppm_state = PPM_STATE_IDLE;
uint8_t cur_ppm_channel = 0;
uint8_t cur_ppm_output = LOW;
int usedFrameLength = 0;
static bool ppmInverted = false;
bool isPpmRunning = false;

void IRAM_ATTR onPpmTimer()
{
    int currentChannelValue;
    portENTER_CRITICAL(&timerMux);
    cur_ppm_output = !cur_ppm_output;
    if (cur_ppm_state == PPM_STATE_IDLE)
    {
        cur_ppm_output = ppmInverted ? HIGH : LOW; // 空闲时输出低电平
        cur_ppm_state = PPM_STATE_SYNC;
        cur_ppm_channel = 0;
        usedFrameLength = 0;
    }

    if (cur_ppm_state == PPM_STATE_SYNC)
    {
        cur_ppm_output = ppmInverted ? LOW : HIGH; // Sync脉冲输出高电平
        usedFrameLength += ppmSyncLenth;
        cur_ppm_state = PPM_STATE_CHANNEL;
        if (cur_ppm_channel >= channels)
        {
            cur_ppm_state = PPM_STATE_FINALL;
        }
        timerAlarmWrite(timer, ppmSyncLenth, true);
    }
    else if (cur_ppm_state == PPM_STATE_CHANNEL)
    {
        cur_ppm_output = ppmInverted ? HIGH : LOW; // Channel值输出低电平
        currentChannelValue = channelValue[cur_ppm_channel];
        cur_ppm_channel++;
        cur_ppm_state = PPM_STATE_SYNC;
        usedFrameLength += currentChannelValue - ppmSyncLenth;
        timerAlarmWrite(timer, currentChannelValue - ppmSyncLenth, true);
    }
    else if (cur_ppm_state == PPM_STATE_FINALL)
    {
        cur_ppm_output = ppmInverted ? HIGH : LOW; // 最后输出输出低电平
        cur_ppm_state = PPM_STATE_IDLE;
        timerAlarmWrite(timer, frameLength - usedFrameLength, true);
    }
    portEXIT_CRITICAL(&timerMux);
    digitalWrite(OUTPUT_PIN, cur_ppm_output);
}

uint8_t autoCalcTimerDivider()
{
    uint32_t cpuFreqMhz = getCpuFrequencyMhz();            // 动态获取CPU主频（单位：MHz）
    uint8_t divider = cpuFreqMhz / TARGET_TIMER_CLOCK_MHZ; // 分频系数 = CPU主频 / 目标定时器时钟（1MHz）

    // 安全校验：分频系数必须是正整数，且不超过ESP32定时器最大分频（256）
    if (divider == 0 || divider > 32767)
    {
        Serial.printf("CPU Frequency(%dMHz) can not set to 1MHz timeer，use default timer divider 80\n", cpuFreqMhz);
        divider = 80; //  fallback到默认值
    }

    Serial.printf("auto divider：CPU Frequency=%dMHz → divider=%d\n", cpuFreqMhz, divider);
    return divider;
}

void init_ppm_timer()
{
    timeDivider = autoCalcTimerDivider();
    timer = timerBegin(0, timeDivider, true);
    timerAttachInterrupt(timer, &onPpmTimer, true);
    timerAlarmWrite(timer, 12000, true);
}

void ppm_init(uint8_t sPin, uint8_t sChannels)
{
    Serial.println("PPM output initialized ...");
    ppmPin = sPin;
    setPPMChannels(sChannels);
    pinMode(OUTPUT_PIN, OUTPUT);
    init_ppm_timer();
    Serial.printf("PPM output initialized successfully on pin %d \n", OUTPUT_PIN);
}

// 启动/停止PPM信号（isStop=true：停止；isStop=false：启动）
void ppmStartAndstop(bool isStop)
{
    portENTER_CRITICAL(&timerMux); // 加锁避免中断冲突
    if (timer == NULL)
    {
        portEXIT_CRITICAL(&timerMux);
        Serial.println("Error: PPM not initialized! Call ppm_init() first.");
        return;
    }

    if (isStop)
    {
        if (isPpmRunning)
        {
            timerAlarmDisable(timer); // 禁用定时器闹钟，中断停止
            isPpmRunning = false;
            Serial.println("PPM output stopped.");
        }
    }
    else
    {
        if (!isPpmRunning)
        {
            // 启动时重置状态机（可选：避免从上次中断状态继续，确保帧完整）
            cur_ppm_state = PPM_STATE_IDLE;
            cur_ppm_channel = 0;
            cur_ppm_output = ppmInverted ? HIGH : LOW;
            usedFrameLength = 0;
            timerAlarmEnable(timer); // 启用定时器闹钟，恢复中断
            isPpmRunning = true;
            Serial.println("PPM output started.");
        }
    }
    portEXIT_CRITICAL(&timerMux);
}

// 重置所有通道到中立点
void resetPPMChannels()
{
    portENTER_CRITICAL(&timerMux); // 加锁保护

    for (uint8_t i = 0; i < channels; i++)
    {
        channelValue[i] = DEFAULT_CHANNEL_VALUE;
    }
    portEXIT_CRITICAL(&timerMux);
}

void setPPMChannelValues(uint16_t *values)
{
    portENTER_CRITICAL(&timerMux); // 加锁保护
    for (uint8_t i = 0; i < channels; i++)
    {
        if (values[i] >= MAX_CHANNEL_VALUE)
        {
            channelValue[i] = MAX_CHANNEL_VALUE;
        }
        else if (values[i] <= MIN_CHANNEL_VALUE)
        {
            channelValue[i] = MIN_CHANNEL_VALUE;
        }
        else
        {
            channelValue[i] = values[i];
        }
    }
    portEXIT_CRITICAL(&timerMux);
}

void setPPMChannels(uint8_t sChannels)
{
    if (sChannels > 8 || sChannels <= 0)
    {
        channels = 8;
    }
}

void setPPMFrameLength(uint32_t sFrameLength)
{
    if (sFrameLength > PPM_MAX_FRAME_LENGTH)
    {
        frameLength = PPM_MAX_FRAME_LENGTH;
    }
    else if (sFrameLength < PPM_MIN_FRAME_LENGTH)
    {
        frameLength = PPM_MIN_FRAME_LENGTH;
    }
    else
    {
        frameLength = sFrameLength;
    }
}

void setPPMSyncLength(uint16_t sPPMSyncLength)
{
    if (sPPMSyncLength > PPM_MAX_SYNC_LENGTH)
    {
        ppmSyncLenth = PPM_MAX_SYNC_LENGTH;
    }
    else if (sPPMSyncLength < PPM_MIN_SYNC_LENGTH)
    {
        ppmSyncLenth = PPM_MIN_SYNC_LENGTH;
    }
    else
    {
        ppmSyncLenth = sPPMSyncLength;
    }
}
