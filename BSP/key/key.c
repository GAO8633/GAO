#include  "key.h"


volatile TickType_t Time;            // 定时器计数变量，用于记录 Echo 为高电平的时间周期数

// /**
//  * @brief  初始化 TIM4 定时器，用于计时 Echo 引脚高电平持续时间
//  * @note   定时器时钟来源为内置时钟，计数周期：ARR=7199，PSC=0 可得每次更新中断间隔 1ms
//  */
// void TIM4_Init(void)
// {
//     // 使能 TIM4 时钟
//     RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
 
//     // 配置为内部时钟模式
//     TIM_InternalClockConfig(TIM4);
 
//     // 定时器基础参数配置
//     TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
//     TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;          // 不分频
//     TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;      // 向上计数模式
//     TIM_TimeBaseInitStructure.TIM_Period = 1000-1;                         // 自动重装载寄存器 (ARR)，每一毫秒进入中断
//     TIM_TimeBaseInitStructure.TIM_Prescaler = 72-1;                      // 预分频器 (PSC)，CNT单位为一微秒
//     TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;                 // 仅在高级定时器中有效，普通定时器置 0
//     TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);
 
//     // 清除更新中断标志
//     TIM_ClearFlag(TIM4, TIM_FLAG_Update);
//     // 使能更新中断
//     TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
 
//     // 中断优先级配置
//     NVIC_InitTypeDef NVIC_InitStructure;
//     NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;                      // 定时器 4 中断通道
//     NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                      // 使能中断
//     NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;            // 抢占优先级
//     NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;                   // 响应优先级
//     NVIC_Init(&NVIC_InitStructure);
 
//     // 启动定时器
//     TIM_Cmd(TIM4, ENABLE);				//定时器只有在中断发生且为上升沿的时候开始计时
// }

// void TIM4_IRQHandler(void)
// {
// 	if (TIM_GetITStatus(TIM4,TIM_IT_Update) == SET)						//UPDATE为更新中断
// 	{
//  		TIM_ClearITPendingBit(TIM4,TIM_IT_Update);
// 		Time++;															//time为定时器4进入中断的次数，单位一毫秒
// 	}
// }


//存在两个按键
static Key_t g_key1 ,g_key2;

/* 读取按键1电平，由于按下是低电平，所以空闲状态是高电平，要配置上拉输入模式。
 * 如果按下了，那就返回1，否则返回0 
*/
static u8 Key1_ReadPin(void)
{
    return (KEY1_READ() == KEY_PRESSED_LEVEL) ? 1 : 0;
}

/* 读取按键2电平 */
static u8 Key2_ReadPin(void)
{
    return (KEY2_READ() == KEY_PRESSED_LEVEL) ? 1 : 0;
}

/* 初始化按键结构体 */
void Key_Init(void)
{
    io_set(KEY1_GPIO_PORT, KEY1_GPIO_PIN, GPIO_Mode_IPU);  /* 配置按键1引脚为上拉输入 */
    io_set(KEY2_GPIO_PORT, KEY2_GPIO_PIN, GPIO_Mode_IPU);  /* 配置按键2引脚为上拉输入 */

    // TIM4_Init();  /* 初始化定时器4，用于按键事件的时间判断 */

    /* 初始化按键1结构体 */
    g_key1.state = KEY_STATE_IDLE;
    g_key1.pressStartTick = 0; 
    g_key1.readPin = Key1_ReadPin;
    g_key1.event = KEY_EVENT_NONE;
    g_key1.longPressTriggered = 0;

    /* 初始化按键2结构体 */
    g_key2.state = KEY_STATE_IDLE;
    g_key2.pressStartTick = 0;
    g_key2.readPin = Key2_ReadPin;
    g_key2.event = KEY_EVENT_NONE;
    g_key2.longPressTriggered = 0;    

}

/**
 * @brief 按键状态机处理（单个按键）
 * @param key: 按键结构体指针
 * @note 使用Time实现精确时间判断，不受调用周期影响
 *       主要用两个key参数 state和event。state用于状态转换，而关键就是状态转换过程的event输出。
 *       最后是将key.event的值传递给主循环，主循环根据event的值来执行相应的操作。
 */
static void Key_StateMachine(Key_t *key)
{
    uint8_t keyPressed = key->readPin();  /* 读取当前引脚状态 */
    TickType_t currentTick = xTaskGetTickCount();  /* 获取当前系统时间戳 */
    TickType_t elapsedTime;   /* 计算按键按下持续时间 */
    switch(key->state)
    {
        case KEY_STATE_IDLE:
            if (keyPressed)  /* 检测到按键按下 */
            {
                key->state = KEY_STATE_DEBOUNCE;  /* 进入消抖状态 */
                key->pressStartTick = currentTick;  /* 记录按下开始时间戳 */
            }
            break;
        case KEY_STATE_DEBOUNCE:
            if (keyPressed)  /* 持续检测到按键按下 */
            {
                elapsedTime = currentTick - key->pressStartTick;  /* 计算消抖时间 */
                if (elapsedTime >= DEBOUNCE_TIME_MS)  /* 消抖时间满足条件 */
                {
                    key->state = KEY_STATE_PRESSED;  /* 确认按键按下 */
                    key->longPressTriggered = 0;  /* 重置长按触发标志 */
                    key->pressStartTick = currentTick;  /* 记录按下开始时间戳 */
                }
            }
            else
            {
                key->state = KEY_STATE_IDLE;  /* 按键未持续按下，返回空闲状态 */
            }
            break;
        case KEY_STATE_PRESSED:
            if (keyPressed)  /* 持续检测到按键按下 */
            {
                elapsedTime = currentTick - key->pressStartTick;  /* 计算按键持续时间 */
                if (elapsedTime >= LONG_PRESS_TIME_MS )  /* 长按时间满足条件且未触发过长按事件 */
                {
                    key->state = KEY_STATE_LONG_PRESS;  /* 转入长按状态 */
                    key->event = KEY_EVENT_LONG_PRESS;  /* 输出长按事件 */
                    key->longPressTriggered = 1;  /* 设置长按触发标志，防止重复触发长按事件 */
                }
            }
            else
            {
               /* 释放按键，不仅判断是否存在短按事件，同时为长按事件结束进行恢复 */
                if (!key->longPressTriggered)
                {
                    /* 未触发长按，则为短按 */
                    key->event = KEY_EVENT_SHORT_PRESS;
                }
                key->state = KEY_STATE_IDLE;
                key->longPressTriggered = 0;
            }
            break;
        case KEY_STATE_LONG_PRESS:
            if (keyPressed)  /* 持续检测到按键按下 */
            {
                key->event = KEY_EVENT_LONG_PRESSING;  /* 输出长按持续中事件 */
            }
            else
            {
                key->state = KEY_STATE_IDLE;  /* 按键释放，返回空闲状态 */
                key->longPressTriggered = 0;  /* 重置长按触发标志 */
                key->event = KEY_EVENT_RELEASE;  /* 输出按键释放事件 */
            }
            break;
        default:
            key->state = KEY_STATE_IDLE;
            break;
    }
}

/**
 * @brief 按键扫描函数（需周期性调用），将两个按键的状态机处理函数放在一起管理
 * @note Key_StateMachine函数static修饰，限制在本文件内使用。
 */
void Key_Scan(void)
{
    Key_StateMachine(&g_key1);  /* 扫描按键1状态机 */
    Key_StateMachine(&g_key2);  /* 扫描按键2状态机 */
}


/**
 * @brief 获取按键1事件
 * @return 按键事件类型
 */
KeyEvent_t Key1_GetEvent(void)
{
    return g_key1.event;
}

/**
 * @brief 获取按键2事件
 * @return 按键事件类型
 */
KeyEvent_t Key2_GetEvent(void)
{
    return g_key2.event;
}

/**
 * @brief 清除按键1事件
 */
void Key1_ClearEvent(void)
{
    g_key1.event = KEY_EVENT_NONE;
}

/**
 * @brief 清除按键2事件
 */
void Key2_ClearEvent(void)
{
    g_key2.event = KEY_EVENT_NONE;
}

/**
 * @brief 检查按键1是否处于按下状态
 * @return 1:按下; 0:未按下
 */
u8 Key1_IsPressed(void)
{
    return (g_key1.state == KEY_STATE_PRESSED || 
            g_key1.state == KEY_STATE_LONG_PRESS) ? 1 : 0;
}

/**
 * @brief 检查按键2是否处于按下状态
 * @return 1:按下; 0:未按下
 */
u8 Key2_IsPressed(void)
{
    return (g_key2.state == KEY_STATE_PRESSED || 
            g_key2.state == KEY_STATE_LONG_PRESS) ? 1 : 0;
}


