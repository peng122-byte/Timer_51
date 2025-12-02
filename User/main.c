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
