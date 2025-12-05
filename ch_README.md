# 蓝牙头部追踪接收器

基于ESP32-C3的蓝牙低功耗（BLE）头部追踪接收器，可将OpenTX/EdgeTX遥控器的头部追踪数据转换为PPM（脉冲位置调制）信号，用于FPV无人机控制。

## 功能特性

- **蓝牙低功耗（BLE）连接**：接收OpenTX/EdgeTX兼容遥控器的头部追踪数据
- **PPM信号输出**：将接收到的数据转换为标准PPM信号供飞控输入
- **多通道支持**：支持多达8个通道的头部追踪数据
- **FreeRTOS任务管理**：通过独立任务高效管理蓝牙、LED控制和数据同步
- **LED状态指示**：提供连接状态和操作状态的视觉反馈
- **ESP32-C3平台**：基于经济高效的ESP32-C3开发板构建

## 硬件需求

- ESP32-C3-DevKitC-02开发板
- PPM信号输出电路（连接到指定GPIO引脚）
- LED指示灯
- 电源供应（通过USB或外部5V供电）

## 软件需求

- PlatformIO IDE或带PlatformIO扩展的VS Code
- ESP32平台支持
- Arduino框架

## 引脚配置

- **PPM输出**：GPIO 6（可配置）
- **LED控制**：通过LED任务配置
- **串口调试**：启用USB CDC启动

## 安装步骤

1. 克隆本仓库或下载源代码
2. 在PlatformIO中打开项目
3. 安装所需的依赖库（如果有）
4. 构建项目
5. 上传到ESP32-C3开发板
6. 将PPM输出连接到飞控的训练端口

## 使用方法

1. 给ESP32-C3开发板通电
2. 设备将通过BLE开始广播
3. 与OpenTX/EdgeTX遥控器配对
4. 在遥控器上配置头部追踪
5. 设备将输出对应头部运动的PPM信号

## 项目结构

```
BlueToothHeadTrackerReceiver/
├── src/
│   ├── main.cpp          # 主应用程序，创建FreeRTOS任务
│   ├── bluetooth.cpp     # BLE通信和数据接收
│   ├── ppmout.cpp        # PPM信号生成和输出
│   ├── led.cpp           # LED状态指示
│   └── opentxbt.cpp      # OpenTX/EdgeTX协议处理
├── include/
│   ├── bluetooth.h       # 蓝牙任务和数据结构
│   ├── ppmout.h          # PPM输出函数声明
│   ├── led.h             # LED控制定义
│   └── opentxbt.h        # OpenTX协议定义
├── platformio.ini        # PlatformIO项目配置
└── README文件            # 文档说明
```

## 配置说明

### PPM设置
- **通道数**：支持8个通道
- **帧长度**：可通过`setPPMFrameLength()`配置
- **同步长度**：可通过`setPPMSyncLength()`配置
- **输出引脚**：在`ppm_init()`中可配置

### 蓝牙设置
- **设备名称**：在bluetooth.cpp中可配置
- **服务UUID**：为OpenTX/EdgeTX兼容性定义
- **特征UUID**：用于头部追踪数据接收

## FreeRTOS任务

1. **蓝牙任务**：处理BLE连接和数据接收
2. **LED任务**：管理状态LED指示灯
3. **主循环**：最小化处理，主要用于任务监督

## 支持的协议

- OpenTX/EdgeTX训练协议（通过BLE）
- 标准PPM信号格式
- 8通道头部追踪数据

## 故障排除

### 连接问题
- 确保遥控器上启用了BLE
- 检查设备是否正在广播
- 验证与遥控器的UUID兼容性

### PPM信号问题
- 检查与飞控的接线连接
- 验证PPM帧时序设置
- 使用示波器调试信号质量

### 构建问题
- 确保ESP32平台已正确安装
- 检查串口输出的USB CDC配置
- 验证GPIO引脚分配与硬件匹配

## 贡献

欢迎贡献！请随时提交拉取请求或打开问题报告错误和请求新功能。

## 许可证

本项目是开源的。请查看仓库获取具体的许可证信息。

## 致谢

- OpenTX/EdgeTX社区提供协议规范
- ESP32 Arduino社区提供平台支持
- FPV社区提供测试和反馈