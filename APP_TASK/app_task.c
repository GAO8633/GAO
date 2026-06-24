#include "app_task.h"
#include "stm32f10x.h"                  // Device header
#include "motor.h"
#include "lcd.h"
#include "gui.h"
#include "Delay.h"
#include "stdio.h"
#include "pid.h"
#include "stdio.h"
#include "key.h"
#include "beep.h"
#include "DHT11.h"
#include "mq2.h"
#include "windspeed.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/*-----------------------------------------------------------
 * 全局变量定义1
 *----------------------------------------------------------*/

SystemState_t g_systemState; /* 系统状态结构体 */
SemaphoreHandle_t g_dataMutex = NULL;       /* 数据互斥信号量 */
SemaphoreHandle_t g_speedCalcSemaphore = NULL;  /* 速度计算二值信号量 */
SemaphoreHandle_t g_iapSemaphore = NULL;      /* IAP信号量 */

volatile int overflow=0;			/* 溢出计数器 */
volatile float speed;				/*电机转速*/

/*Key变量声明*/
led_d bep;							/*beep结构体变量--不甘心的咸鱼注*/
KeyEvent_t key1Event, key2Event;

/* PID速度环控制相关变量 */
PID_TypeDef g_speedPID;             /* 速度环PID控制器 */

led_d dht;							/*DHT11结构体变量*/



WindSpeed_t g_windSpeedData; /* 风速算法数据结构 */


/* 任务句柄 */
static TaskHandle_t xStartTaskHandle = NULL;
static TaskHandle_t xKeyScanTaskHandle = NULL;
static TaskHandle_t xSensorTaskHandle = NULL;
static TaskHandle_t xWindSpeedTaskHandle = NULL;
static TaskHandle_t xMotorControlTaskHandle = NULL;
static TaskHandle_t xUIDisplayTaskHandle = NULL;
static TaskHandle_t xAntiBackflowTaskHandle = NULL;
static TaskHandle_t xSpeedCalcTaskHandle = NULL;


#if ifopen
	static TaskHandle_t xIAPTaskHandle = NULL;
#endif


/* PID控制器 */
extern PID_TypeDef g_speedPID;


/* 自动模式状态机状态 */
typedef enum {
    AUTO_STATE_STARTUP,             /* 启动阶段（等待cooking event） */
    AUTO_STATE_COOKING,             /* Cooking Event激活 */
    AUTO_STATE_DELAY_OFF            /* 延时关闭阶段 */
} AutoModeState_t;

static AutoModeState_t g_autoModeState = AUTO_STATE_STARTUP;

#if ifopen  /*固件升级功能，右键点击该宏--goto definition--将0改成1即可使用*/   
/*****************************************************************************************************************************************************/
/**定义APP程序缓冲区大小*/
/**若使能为1，此时点击MAP文件会发现RAM大小为19.56KB，这是因为boot偏移了16K，再加上APP的3.56K，因此这里的RAM是boot加APP的合计***/
/*需要注意不能让bootloader程序的RAM区域与APP程序的RAM区域重叠，而笔者bootloader的RAM大概为13K(忽略掉buff_size编译所得的ram大小就是boot区的ram大小)，
*因此偏移了16K，为0x4000,剩下的4Kram给APP*/
/******************************************************************************************************************************************************/

u8 receive_buff[buff_size]  __attribute__ ((at(0X20004000)));/*将数组分配到固定的内存地址（请根据需要修改）,否则会判断固件程序失败*/
	
#endif

/*-----------------------------------------------------------
 * 模式名称字符串
 *----------------------------------------------------------*/
static const char* ModeNames[] = {
    "Standby",
    "Manual ",
    "Auto   ",
    "Anti-BF"
};

static const char* SpeedLevelNames[] = {
    "LOW ",
    "HIGH"
};

static const char* AutoStateNames[] = {
    "Startup ",
    "Cooking ",
    "DelayOff"
};

/*-----------------------------------------------------------
 * 系统初始化
 *----------------------------------------------------------*/
void System_Init(void)
{
    /* 初始化系统状态 */
    g_systemState.currentMode = MODE_STANDBY;
    g_systemState.speedLevel = SPEED_LOW;
    g_systemState.motorRunning = 0;
    g_systemState.temperature = 0;
    g_systemState.humidity = 0;
    g_systemState.gasConcentration = 0.0f;
    g_systemState.windSpeedPWM = 0.0f;
    g_systemState.actualRPM = 0.0f;
    g_systemState.targetRPM = 0;
    g_systemState.cookingEventActive = 0;
    g_systemState.antiBackflowActive = 0;
    g_systemState.gasThreshold = GAS_THRESHOLD_NORMAL;
    g_systemState.autoModeCounter = 0;
    g_systemState.cookingEventCounter = 0;
     
    /* 创建互斥信号量 */
    g_dataMutex = xSemaphoreCreateMutex();
    
    /* 创建速度计算二值信号量 */
    g_speedCalcSemaphore = xSemaphoreCreateBinary();

#if ifopen
    g_iapSemaphore = xSemaphoreCreateBinary();
#endif

}

/*-----------------------------------------------------------
 * 创建开始任务
 *----------------------------------------------------------*/
void StartTask_Create(void)
{
    xTaskCreate(StartTask, "StartTask", TASK_START_STK_SIZE, NULL, 
                TASK_START_PRIORITY, &xStartTaskHandle);
}

void StartTask(void *pvParameters)
{

    taskENTER_CRITICAL();   /* 进入临界区 */
    
    /* 创建按键扫描任务 */
    xTaskCreate(KeyScanTask, "KeyScan", TASK_KEY_STK_SIZE, NULL,
                TASK_KEY_PRIORITY, &xKeyScanTaskHandle);
    
    /* 创建传感器采集任务 */
    xTaskCreate(SensorTask, "Sensor", TASK_SENSOR_STK_SIZE, NULL,
                TASK_SENSOR_PRIORITY, &xSensorTaskHandle);
    
    /* 创建风速计算任务 */
    xTaskCreate(WindSpeedTask, "WindSpeed", TASK_WIND_SPEED_STK_SIZE, NULL,
                TASK_WIND_SPEED_PRIORITY, &xWindSpeedTaskHandle);
    
    /* 创建电机控制任务 */
    xTaskCreate(MotorControlTask, "Motor", TASK_MOTOR_STK_SIZE, NULL,
                TASK_MOTOR_PRIORITY, &xMotorControlTaskHandle);
    
    /* 创建UI显示任务 */
    xTaskCreate(UIDisplayTask, "UI", TASK_UI_STK_SIZE, NULL,
                TASK_UI_PRIORITY, &xUIDisplayTaskHandle);
    
    /* 创建防回流任务 */
    xTaskCreate(AntiBackflowTask, "AntiBF", TASK_ANTI_BACKFLOW_STK_SIZE, NULL,
                TASK_ANTI_BACKFLOW_PRIORITY, &xAntiBackflowTaskHandle);
    
    /* 创建速度计算任务 */
    xTaskCreate(SpeedCalcTask, "SpeedCalc", TASK_SPEED_CALC_STK_SIZE, NULL,
                TASK_SPEED_CALC_PRIORITY, &xSpeedCalcTaskHandle);

    #if ifopen  
    /* 创建IAP任务 */
    xTaskCreate(iap_task, "IAP", TASK_IAP_STK_SIZE, NULL,
                TASK_IAP_PRIORITY, &xIAPTaskHandle);
#endif                
    
    /* 初始化TIM4定时器（必须在信号量创建后调用，避免HardFault）
     * 参数：5-1=4, 14400-1=14399
     * 定时器频率 = 72MHz / 14400 = 5kHz
     * 中断周期 = 5 / 5kHz = 1ms */
    TIM4_init(5-1, 14400-1);
    
    taskEXIT_CRITICAL();    /* 退出临界区 */
    
    /* 删除开始任务 */
    vTaskDelete(xStartTaskHandle);
}

void KeyScanTask(void *pvParameters)
{
    while(1)
    {
        Key_Scan();  /* 扫描按键状态机 */
        
        /* 获取按键1事件（模式切换） */
        key1Event = Key1_GetEvent();
        
        if (key1Event == KEY_EVENT_SHORT_PRESS)
        {
            System_SwitchMode();
            Buzzer_Beep(100,&bep);           /* 短按提示音 */
            Key1_ClearEvent();

        }
        else if (key1Event == KEY_EVENT_LONG_PRESSING)
        {
            Beep_on(&bep);                /* 长按持续鸣叫 */
        }
        else if (key1Event == KEY_EVENT_RELEASE)
        {
            Beep_off(&bep);               /* 释放时停止鸣叫 */
            Key1_ClearEvent();
        }
        
        /* 获取按键2事件（档位切换/风机开关） */
        key2Event = Key2_GetEvent();
        if (key2Event == KEY_EVENT_SHORT_PRESS)
        {
            System_SwitchSpeedLevel();  
            Buzzer_Beep(100,&bep);           /* 短按提示音 */
            Key2_ClearEvent();
        }
        else if (key2Event == KEY_EVENT_LONG_PRESS)
        {
            System_ToggleMotor();         /*长按时关闭或者打开电机，无论电机在何种模式下*/
        
        }
        else if (key2Event == KEY_EVENT_LONG_PRESSING)
        {
            Beep_on(&bep);                /* 长按持续鸣叫 */
        }
        else if (key2Event == KEY_EVENT_RELEASE)
        {
            Beep_off(&bep);               /* 释放时停止鸣叫 */
            Key2_ClearEvent();
        }
        
        Delay_ms(10);   				 /* 10ms扫描周期 */
    }
}




void MotorControlTask(void *pvParameters)
{
    float pidOut;
    while(1)
    {
        u16 pwmCompare = 0;

        /*获取实际转速*/
        if (xSemaphoreTake(g_dataMutex, portMAX_DELAY) == pdTRUE)
        {
            g_systemState.actualRPM = speed;
            xSemaphoreGive(g_dataMutex);
            
        }
        
        switch(g_systemState.currentMode)
        {
            /* 源代码中没有添加互斥锁进行保护，可以考虑添加 */
            case MODE_STANDBY:
            /*待机模式：电机停止，但仍计算风速*/
                motor_stop();
                g_systemState.motorRunning = 0;
                g_systemState.autoModeCounter = 0;
                g_systemState.cookingEventCounter = 0;
                g_autoModeState = AUTO_STATE_STARTUP;
                break;

            case MODE_MANUAL:

                /* 手动模式：PID控制电机转速 */
                if(g_systemState.motorRunning == 0)
                {
                    motor_start();
                    g_systemState.motorRunning = 1;
                }

                if (g_systemState.motorRunning) 
                {
                    /*1.设置目标转速;2.计算PID输出;3.设置电机PWM*/
                    //g_systemState.targetRPM = WindSpeed_GetTargetRPM(g_systemState.speedLevel);
                    /* 调试电机PID 只更改了这个地方 */
                    g_systemState.targetRPM = gao_WindSpeed_GetTargetRPM(g_systemState.speedLevel);                    
                    PID_SetTarget(&g_speedPID, (float)g_systemState.targetRPM);
                    pidOut=PID_Calculate(&g_speedPID, speed);
                    motor_pwm_set(pidOut);

                    

                }
                break;

            case MODE_AUTO:
            /* 自动模式状态机 */
                switch (g_autoModeState)
                {
                    case AUTO_STATE_STARTUP:
                        /* 启动阶段：以最小转速运行，等待Cooking Event */
                        if (!g_systemState.motorRunning)
                        {
                            g_systemState.motorRunning = 1;
                            motor_start();
                        }

                        /* 使用最小转速代表自动模式开启 */                       
                        {
                            motor_pwm_set(PWM_MIN*10);
                        }
                        
                        g_systemState.autoModeCounter += 50;
                        
                        if (g_systemState.cookingEventActive)
                        {
                            /* 检测到Cooking Event */
                            g_autoModeState = AUTO_STATE_COOKING;
                        }
                        else if (g_systemState.autoModeCounter >= AUTO_MODE_STARTUP_TIME)
                        {
                            /* 60秒内无Cooking Event，关闭自动模式 */
                            g_systemState.currentMode = MODE_STANDBY;
                            motor_stop();
                            g_systemState.motorRunning = 0;
                        }
                        break;
                        
                    case AUTO_STATE_COOKING:
                        /* Cooking Event激活：根据传感器自动调节 */
                        {
                            /*根据风速算法设置速度，MAXCCR为最大占空比*/
                            pwmCompare = WindSpeed_GetPWMCCR(MAXCCR);
                            motor_pwm_set(pwmCompare);
                        }
                        
                        g_systemState.cookingEventCounter += 50;
                        
                        if (!g_systemState.cookingEventActive)
                        {
                            /* Cooking Event结束 */
                            g_autoModeState = AUTO_STATE_DELAY_OFF;
                            g_systemState.cookingEventCounter = 0;
                        }
                        else if (g_systemState.cookingEventCounter >= COOKING_EVENT_TIMEOUT)
                        {
                            /* Cooking Event持续超过60秒，关闭自动模式 */
                            g_systemState.currentMode = MODE_STANDBY;
                            motor_stop();
                            g_systemState.motorRunning = 0;
                        }
                        break;
                        
                    case AUTO_STATE_DELAY_OFF:
                        /* 延时关闭阶段 */
                        {
                            /*依据风速输出转速，MAXCCR为最大占空比*/
                            pwmCompare = WindSpeed_GetPWMCCR(MAXCCR);
                            motor_pwm_set(pwmCompare);
                        }
                        
                        g_systemState.cookingEventCounter += 50;
                        
                        if (g_systemState.cookingEventActive)
                        {
                            /* 又检测到Cooking Event */
                            g_autoModeState = AUTO_STATE_COOKING;
                            g_systemState.cookingEventCounter = 0;
                        }
                        else if (g_systemState.cookingEventCounter >= COOKING_EVENT_DELAY_OFF)
                        {
                            /* 10秒延时结束，关闭自动模式 */
                            g_systemState.currentMode = MODE_STANDBY;
                            motor_stop();
                            g_systemState.motorRunning = 0;
                        }
                        break;
                }
                break;

            case MODE_ANTI_BACKFLOW:
            /* 防回流模式：由防回流任务控制 */
                if (g_systemState.antiBackflowActive && g_systemState.motorRunning)
                {
                    /* 使用用户设定的档位转速 */
                    g_systemState.targetRPM = WindSpeed_GetTargetRPM(g_systemState.speedLevel);
                    PID_SetTarget(&g_speedPID, (float)g_systemState.targetRPM);
                    pidOut = PID_Calculate(&g_speedPID, speed);
                    motor_pwm_set(pidOut);
                }
                break;
            
        }
        /* 显示实时转速进行调试 */
        printf("%.1f,%.1f,%.1f\n", (float)g_systemState.targetRPM,
                                    g_systemState.actualRPM,
                                    pidOut);
        
        Delay_ms(50);  /* 50ms控制周期 */
    }
}

/*   这个在CPU进行执行 会不会慢？   */
void SpeedCalcTask(void *pvParameters)
{
    int encoderCount;
    while(1)
    {
        /* 等待TIM4中断发送的信号量 */
        if (xSemaphoreTake(g_speedCalcSemaphore, portMAX_DELAY) == pdTRUE)
    	{
            /* 获取当前编码器计数值 */
            encoderCount = get_encoder_value();
            
            /* 计算电机转速，采样精度为50ms */
            speed = get_speed(encoderCount, 50);
    	}
    }	
}

/*-----------------------------------------------------------
 * 防回流任务 - 周期100ms
 *----------------------------------------------------------*/
void AntiBackflowTask(void *pvParameters)
{
    u8 isDetected = 0;  

    while (1)
    {
        /* 判断是否是防回流模式 */
        if (g_systemState.currentMode == MODE_ANTI_BACKFLOW)
        {
            /* gasThreshold当前设置为100 */
            isDetected = (g_systemState.gasConcentration >= g_systemState.gasThreshold) ? 1 : 0;

            if(isDetected)
            {
                if(g_systemState.gasConcentration < GAS_THRESHOLD_HIGH)/*防止在极端情况下会重启风机*/
                {
                        /* 气体超过NORMAL阈值，启动风机 */
                        g_systemState.antiBackflowActive = 1;
                        g_systemState.motorRunning = 1;
                        motor_start();
                        
                        /* 立即切换到HIGH阈值，防止重复触发 */
                        g_systemState.gasThreshold = GAS_THRESHOLD_HIGH;
                }
            }
            else if(g_systemState.gasConcentration < GAS_THRESHOLD_NORMAL)
            {
                /* 气体浓度降到NORMAL阈值以下，停止风机 */
                g_systemState.antiBackflowActive = 0;
                g_systemState.motorRunning = 0;
                motor_stop();
                g_systemState.gasThreshold = GAS_THRESHOLD_NORMAL;
            }
            
        }
        else
        {
            /* 非防回流模式，重置所有状态 */
            g_systemState.antiBackflowActive = 0;
            g_systemState.gasThreshold = GAS_THRESHOLD_NORMAL;
        }
        
        delay_ms(100);  /* 100ms检测周期 */
    }
}

void SensorTask(void *pvParameters)
{
    while(1)
    {
        u8 temp,humi;
        float gasValue;
        if (DHT_Read_Data(&temp, &humi, GPIOC, GPIO_Pin_14, &dht)) 
        {
            /*如果此处获取2个传感器数据使用同一把锁，会导致占用资源时间过长，
            更容易带来优先级反转问题；因此此处采用两把锁*/
            if (xSemaphoreTake(g_dataMutex, portMAX_DELAY) == pdTRUE)
            {
                g_systemState.temperature = temp;
                g_systemState.humidity = humi;
                xSemaphoreGive(g_dataMutex);
            }
        /* 读取MQ2数据 */
        {
            gasValue = MQ2_GetGasConcentration();
            if (xSemaphoreTake(g_dataMutex, portMAX_DELAY) == pdTRUE)
            {
                g_systemState.gasConcentration = gasValue;
                xSemaphoreGive(g_dataMutex);
            }
        }
            
        }
#if SENSOR_DEBUG    /*传感器数据调试，右键点击该宏--goto definition--将0改成1即可使用*/        
        printf("Temp: %d, Humi: %d, Gas: %.2f\r\n", temp, humi, gasValue);
#endif
        delay_ms(500);  /* 500ms采集周期 */ 
    }
}

void WindSpeedTask(void *pvParameters)
{
    u8 temp, humi;
    float gasValue;
    
    while(1)
    {
        /* 获取传感器数据 */
        if (xSemaphoreTake(g_dataMutex, portMAX_DELAY) == pdTRUE)
        {
            temp = g_systemState.temperature;
            humi = g_systemState.humidity;
            gasValue = (float)g_systemState.gasConcentration;
            xSemaphoreGive(g_dataMutex);
        }

        /* 更新风速算法数据结构 */
        WindSpeed_Update(temp, humi, gasValue);
        /* 更新系统状态 */
        if (xSemaphoreTake(g_dataMutex, portMAX_DELAY) == pdTRUE)
        {
            g_systemState.windSpeedPWM = WindSpeed_GetPWM();
            g_systemState.cookingEventActive = WindSpeed_IsCookingEvent();
            xSemaphoreGive(g_dataMutex);
        }
        
        delay_ms(100);  /* 100ms计算周期 */
    }
}

void UIDisplayTask(void *pvParameters)
{
    char dispBuf[32];
    /* 清屏 */
    LCD_Clear(WHITE); 

    while(1)
    {
        sprintf(dispBuf, "Mode:%s",ModeNames[g_systemState.currentMode]);
        Show_Str(0, 20, BLUE, WHITE, (u8*)dispBuf, 16, 0);

        /* 显示自动模式状态 */
        sprintf(dispBuf, "Auto:%s", AutoStateNames[g_autoModeState]);
        Show_Str(0, 40, BLUE, WHITE, (u8*)dispBuf, 16, 0);

        sprintf(dispBuf, "temp:%d humi:%d",g_systemState.temperature,g_systemState.humidity);
        Show_Str(0, 60, BLUE, WHITE, (u8*)dispBuf, 16, 0);

        sprintf(dispBuf, "gasValue:%.2f",g_systemState.gasConcentration);
        Show_Str(0, 80, BLUE, WHITE, (u8*)dispBuf, 16, 0);

        sprintf(dispBuf, "AMCnt:%ds CookE:%ds  ", g_systemState.autoModeCounter/1000,g_systemState.cookingEventActive);
        Show_Str(0, 100, BLUE, WHITE, (u8*)dispBuf, 16, 0);

        sprintf(dispBuf, "CookEtime:%ds  ", g_systemState.cookingEventCounter/1000);
        Show_Str(0, 120, BLUE, WHITE, (u8*)dispBuf, 16, 0);
    
        
        /*显示转速*/
        sprintf(dispBuf, "RPM:%.0f   ", speed);
        Show_Str(0, 140, BLUE, WHITE, (u8*)dispBuf, 16, 0);

        // /*显示转速*/
        // sprintf(dispBuf, "PIDout:%.0f   ", pid_output);
        // Show_Str(0, 100, BLUE, WHITE, (u8*)dispBuf, 16, 0);

        Delay_ms(200);  /* 200ms刷新周期 */
    }   
}

/*-----------------------------------------------------------
 * 切换工作模式
 *----------------------------------------------------------*/
void System_SwitchMode(void)
{
    if (xSemaphoreTake(g_dataMutex, portMAX_DELAY) == pdTRUE)
    {
        /* 模式循环切换：待机 -> 手动 -> 自动 -> 防回流 -> 待机 */
        switch (g_systemState.currentMode)
        {
            case MODE_STANDBY:
                g_systemState.currentMode = MODE_MANUAL;
                break;
            case MODE_MANUAL:
                g_systemState.currentMode = MODE_AUTO;
                g_autoModeState = AUTO_STATE_STARTUP;
                break;
            case MODE_AUTO:
                g_systemState.currentMode = MODE_ANTI_BACKFLOW;
                break;
            case MODE_ANTI_BACKFLOW:
                g_systemState.currentMode = MODE_STANDBY;
                break;
        }
        
        /* 切换模式时停止电机（除自动模式外） */
        if (g_systemState.currentMode != MODE_AUTO)
        {
            g_systemState.motorRunning = 0;
            motor_stop();
        }
        
        xSemaphoreGive(g_dataMutex);
    }
}


/*-----------------------------------------------------------
 * 切换档位
 *----------------------------------------------------------*/
void System_SwitchSpeedLevel(void)
{
    if (xSemaphoreTake(g_dataMutex, portMAX_DELAY) == pdTRUE)
    {
        /* 档位循环切换：LOW -> HIGH -> LOW */
        switch (g_systemState.speedLevel)
        {
            case SPEED_LOW:
                g_systemState.speedLevel = SPEED_HIGH;
                break;
            case SPEED_HIGH:
                g_systemState.speedLevel = SPEED_LOW;
                break;
        }
        xSemaphoreGive(g_dataMutex);
    }
}

/*-----------------------------------------------------------
 * 切换电机开关
 *----------------------------------------------------------*/
void System_ToggleMotor(void)
{
    if (xSemaphoreTake(g_dataMutex, portMAX_DELAY) == pdTRUE)
    {
        if (g_systemState.motorRunning)
        {
            /* 关闭电机：强制切换到待机模式，确保MotorControlTask不会重新拉起电机 */
            motor_stop();
            g_systemState.motorRunning = 0;
            g_systemState.currentMode = MODE_STANDBY;
            Show_Str(0, 0, BLUE, WHITE, "Motor Stopped", 16, 0);
        }
        else
        {
            /* 开启电机：强制切换到手动模式，确保MotorControlTask持续驱动电机 */
            motor_start();
            g_systemState.motorRunning = 1;
            g_systemState.currentMode = MODE_MANUAL;
            Show_Str(0, 0, BLUE, WHITE, "Motor Started", 16, 0);
        }
        
        xSemaphoreGive(g_dataMutex);
    }
}


/*-----------------------------------------------------------
 * TIM2中断服务程序 - 编码器溢出处理
 *----------------------------------------------------------*/
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        
        /* 根据计数方向更新溢出计数器 */
        if (TIM_GetDirection(TIM2))
        {
            overflow--;     /* 递减计数 */
        }
        else
        {
            overflow++;     /* 递增计数 */
        }
    }
}

/*-----------------------------------------------------------
 * TIM4中断服务程序 - 触发速度计算任务1ms发生一次中断
 *----------------------------------------------------------*/
void TIM4_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

   if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
   {
       TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
       
        xSemaphoreGiveFromISR(g_speedCalcSemaphore, &xHigherPriorityTaskWoken);

        /* 如果需要，触发上下文切换 */
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        
   }
}


