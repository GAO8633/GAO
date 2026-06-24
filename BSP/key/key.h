#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"                  // Device header
#include "sys.h"                        // 这个头文件包含了系统相关的函数和定义，如延时函数等
#include "Delay.h"                      // 包含延时函数的头文件
#include "gpio.h"
#include "FreeRTOS.h"                    //FreeRTOS使用
#include "task.h"                        //FreeRTOS使用

/* 按键引脚定义 */
#define KEY1_GPIO_PORT      GPIOB
#define KEY1_GPIO_PIN       GPIO_Pin_1

#define KEY2_GPIO_PORT      GPIOB
#define KEY2_GPIO_PIN       GPIO_Pin_12

/* 按键电平读取（假设按下为低电平），空闲的时候是上拉 */
#define KEY1_READ()         GPIO_ReadInputDataBit(KEY1_GPIO_PORT, KEY1_GPIO_PIN)
#define KEY2_READ()         GPIO_ReadInputDataBit(KEY2_GPIO_PORT, KEY2_GPIO_PIN)

/* 按键按下电平 */
#define KEY_PRESSED_LEVEL   0   /* 按下为低电平 */

/* 时间参数定义（单位：毫秒，使用TIM4中断TIme精确计时） */
#define DEBOUNCE_TIME_MS    30      /* 消抖时间：30ms */
#define LONG_PRESS_TIME_MS  1000    /* 长按时间：1000ms（1秒） */

typedef uint32_t TickType_t;

typedef enum
{
    KEY_EVENT_NONE = 0,         /* 无事件 */
    KEY_EVENT_SHORT_PRESS,      /* 短按事件 */
    KEY_EVENT_LONG_PRESS,       /* 长按事件 */
    KEY_EVENT_LONG_PRESSING,    /* 长按持续中（用于蜂鸣器持续鸣叫） */
    KEY_EVENT_RELEASE           /* 按键释放事件 */
} KeyEvent_t;

typedef enum
{
    KEY_STATE_IDLE = 0,         /* 空闲状态 */
    KEY_STATE_DEBOUNCE,         /* 消抖状态 */
    KEY_STATE_PRESSED,          /* 按下确认状态 */
    KEY_STATE_LONG_PRESS,       /* 长按状态 */
    KEY_STATE_WAIT_RELEASE      /* 等待释放状态 */
} KeyState_t;

typedef struct
{
    KeyState_t state;           /* 当前状态 */
    KeyEvent_t event;           /* 当前事件 */
    TickType_t pressStartTick;  /* 按下开始时间戳（用于精确计时） */
    u8 (*readPin)(void);        /* 读取引脚电平函数指针 */ 
    u8 longPressTriggered;      /* 长按事件是否已触发 */
} Key_t;

/* 函数声明 */
void Key_Init(void);             /* 按键初始化函数 */
void Key_Scan(void);             /* 按键扫描函数，需周期性调用 */
KeyEvent_t Key1_GetEvent(void);  /* 获取按键1事件 */
KeyEvent_t Key2_GetEvent(void);  /* 获取按键2事件 */
void Key1_ClearEvent(void);                 /* 清除按键1事件 */
void Key2_ClearEvent(void);                 /* 清除按键2事件 */
u8 Key1_IsPressed(void);                    /* 检查按键1是否按下 */
u8 Key2_IsPressed(void);                    /* 检查按键2是否按下 */




#endif
