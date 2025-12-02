------

# 基于 51 单片机的数字秒表 (51 MCU Digital Stopwatch)

## 📌 项目简介

本项目是一个基于 **AT89C52** 单片机的简易数字秒表系统。项目采用 C 语言编写，利用 Keil 编译，并通过 Proteus 软件进行电路仿真。系统能够实现 00:00 到 59:59 的精确计时，并具备启动/暂停、时间微调及硬件复位功能。
![image-20251202134912661](https://cdn.jsdelivr.net/gh/peng122-byte/my-images@main/img/2025/09/14/image-20251202134912661.png)

```c
#include <reg52.h>

// ================= 硬件引脚定义 =================
#define GPIO_DIG    P0      // 段选数据口
#define GPIO_PLACE  P2      // 位选控制口

sbit KEY_START = P3^0;      // 启动/暂停
sbit KEY_MIN   = P3^1;      // 分钟调整
sbit KEY_SEC   = P3^2;      // 秒数调整

// ================= 全局变量定义 =================
unsigned char code DIG_CODE[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
}; // 共阴极 0-9 段码表

unsigned char DisplayData[4]; // 显示缓冲数组，分别存 分十、分个、秒十、秒个
unsigned char minute = 0;     // 分钟变量 (0-59)
unsigned char second = 0;     // 秒钟变量 (0-59)
unsigned char timer_count = 0;// 定时器计数器
bit is_running = 0;           // 运行状态标志：0-停止，1-运行

// ================= 辅助函数 =================

// 简单的软件延时，用于数码管扫描
void DelayIs(unsigned int t) {
    while(t--);
}

// 简单的毫秒延时，用于按键消抖
void DelayMs(unsigned int ms) {
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 110; j++);
}

// 数据处理函数：将此时的分秒拆解放入显示缓冲区
void UpdateDisplayBuffer() {
    DisplayData[0] = DIG_CODE[minute / 10]; // 分钟十位
    DisplayData[1] = DIG_CODE[minute % 10]; // 分钟个位
    DisplayData[2] = DIG_CODE[second / 10]; // 秒钟十位
    DisplayData[3] = DIG_CODE[second % 10]; // 秒钟个位
}

// 数码管扫描显示函数
void Display() {
    // --- 显示第1位 (左边第一位，分钟十位) ---
    GPIO_PLACE = 0xFE;  // P2.0 低电平选中
    GPIO_DIG = DisplayData[0];
    DelayIs(100);
    GPIO_DIG = 0x00;    // 消隐

    // --- 显示第2位 (分钟个位) ---
    GPIO_PLACE = 0xFD;  // P2.1 低电平选中
    GPIO_DIG = DisplayData[1];
    DelayIs(100);
    GPIO_DIG = 0x00;

    // --- 显示第3位 (秒钟十位) ---
    GPIO_PLACE = 0xFB;  // P2.2 低电平选中
    GPIO_DIG = DisplayData[2];
    DelayIs(100);
    GPIO_DIG = 0x00;

    // --- 显示第4位 (右边第一位，秒钟个位) ---
    GPIO_PLACE = 0xF7;  // P2.3 低电平选中
    GPIO_DIG = DisplayData[3];
    DelayIs(100);
    GPIO_DIG = 0x00;
}

// 按键扫描处理函数
void KeyScan() {
    // 1. 启动/停止键 (P3.0)
    if (KEY_START == 0) {
        DelayMs(10); // 消抖
        if (KEY_START == 0) {
            is_running = !is_running; // 取反运行状态
            while(KEY_START == 0) Display(); // 等待松手，期间保持显示
        }
    }

    // 2. 调整分钟 (P3.1)
    if (KEY_MIN == 0) {
        DelayMs(10);
        if (KEY_MIN == 0) {
            minute++;
            if (minute >= 60) minute = 0;
            UpdateDisplayBuffer(); // 更新显示数据
            while(KEY_MIN == 0) Display(); 
        }
    }

    // 3. 调整秒钟 (P3.2)
    if (KEY_SEC == 0) {
        DelayMs(10);
        if (KEY_SEC == 0) {
            second++;
            if (second >= 60) second = 0;
            UpdateDisplayBuffer(); // 更新显示数据
            while(KEY_SEC == 0) Display();
        }
    }
}

// 定时器0初始化函数 (产生50ms中断，基于12MHz晶振)
void Timer0_Init() {
    TMOD |= 0x01;      // 设置定时器0为模式1 (16位)
    TH0 = (65536 - 50000) / 256; // 装初值 50ms
    TL0 = (65536 - 50000) % 256;
    EA = 1;            // 开总中断
    ET0 = 1;           // 开定时器0中断
    TR0 = 1;           // 启动定时器
}

// ================= 主函数 =================
void main() {
    UpdateDisplayBuffer(); // 初始化显示内容
    Timer0_Init();         // 初始化定时器
    
    while(1) {
        KeyScan(); // 扫描按键
        Display(); // 刷新数码管
    }
}

// ================= 中断服务函数 =================
void Timer0_ISR() interrupt 1 {
    // 重新装载初值 (50ms)
    TH0 = (65536 - 50000) / 256;
    TL0 = (65536 - 50000) % 256;
    
    // 只有在运行状态下才计数
    if (is_running) {
        timer_count++;
        if (timer_count >= 20) { // 50ms * 20 = 1000ms = 1秒
            timer_count = 0;
            second++;
            
            // 进位逻辑
            if (second >= 60) {
                second = 0;
                minute++;
                if (minute >= 60) {
                    minute = 0;
                }
            }
            // 每秒更新一次显示缓冲
            UpdateDisplayBuffer();
        }
    }
}

```



## ✨ 功能特性

- **显示范围**：00分00秒 ~ 59分59秒（溢出后分钟自动归零）。
- **显示器件**：4位共阴极数码管 (7SEG-MPX4-CC)，采用动态扫描方式驱动。
- **按键控制**：
  - **P3.0**：启动 / 暂停计时。
  - **P3.1**：分钟调整（加 1）。
  - **P3.2**：秒钟调整（加 1）。
  - **RST**：系统复位（时间归零）。
- **定时机制**：使用内部定时器 T0 (Timer 0) 产生 50ms 中断，累积 20 次中断为 1 秒。

## 🛠 硬件环境 (Proteus 仿真)

- **核心控制器**：AT89C52 (或 AT89C51)
- **晶振频率**：12MHz (配合代码中的定时器初值)
- **显示模块**：`7SEG-MPX4-CC` (4位共阴极数码管)
- **复位电路**：高电平复位 (10kΩ 下拉电阻 + 10µF 上拉电容 + 按键)
- **排阻**：RESPACK-8 (用于 P0 口上拉，增强驱动能力)

## 🔌 引脚连接说明 (Pin Mapping)

| 单片机引脚      | 连接元件          | 说明                       |
| --------------- | ----------------- | -------------------------- |
| **P0.0 - P0.7** | 数码管段选 (A-DP) | 通过排阻上拉，控制数字笔画 |
| **P2.0**        | 数码管位选 1      | 控制左起第1位 (分钟十位)   |
| **P2.1**        | 数码管位选 2      | 控制左起第2位 (分钟个位)   |
| **P2.2**        | 数码管位选 3      | 控制左起第3位 (秒钟十位)   |
| **P2.3**        | 数码管位选 4      | 控制左起第4位 (秒钟个位)   |
| **P3.0**        | 按键 K1           | 启动 / 暂停                |
| **P3.1**        | 按键 K2           | 分钟调整                   |
| **P3.2**        | 按键 K3           | 秒钟调整                   |
| **RST (Pin 9)** | 复位电路          | 全局复位                   |

## 💻 软件开发环境

- **IDE**: Keil uVision (C51)
- **语言**: C
- **编译器**: C51 Compiler

## 🚀 使用指南

1. **编译代码**：
   - 使用 Keil 打开工程文件。
   - 确保 `main.c` 已添加到 "Source Group 1" 中。
   - 点击 "Rebuild" 生成 `.hex` 文件。
2. **配置仿真**：
   - 打开 Proteus 工程。
   - 双击 AT89C52 芯片。
   - 在 **Program File** 中加载生成的 `.hex` 文件。
   - 确保 **Clock Frequency** 设置为 `12MHz`。
3. **运行操作**：
   - 点击 Proteus 左下角的播放按钮开始仿真。
   - 按下 **P3.0** 键开始计时。
   - 再次按下 **P3.0** 键暂停。
   - 暂停状态下，可使用 **P3.1/P3.2** 调整时间。
   - 按下 **复位键** 可随时清零重置。

## ⚠️ 常见问题与注意事项 (Troubleshooting)

1. **数码管完全不亮？**
   - 检查 Proteus 中的网络标签（Label）是否正确放置在**导线**上，而不是直接贴在引脚上。
   - 检查是否已加载 HEX 文件。
2. **复位键按下无反应/一直复位？**
   - 检查 RST 引脚连接：必须使用**下拉电阻** (电阻接 GND) 和**上拉电容** (电容接 VCC)。
   - 确保电阻阻值不过大（推荐 1kΩ）且已正确接地。
3. **计时速度不准？**
   - 检查代码中的定时器初值是否与仿真设置的晶振频率 (12MHz) 匹配。

------

*Created by [pzk] - 2025*