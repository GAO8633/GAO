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
#include "app_task.h"




static void Hardware_Init(void)
{
    /************************************************************************************************************/
	/*由于共计4个中断，设置中断分组为4，4位抢占优先级，0位亚优先级，因为Free不支持亚优先级处理。
	 由于Free中存在屏蔽优先级阈值的概念，该工程笔者设置为3，因此我们需要调用API函数的中断优先级不能超过3，
	 所以DMA中断优先级为4，定时器4中断优先级为5，TIM2中断优先级为6，从优先级都设置为0--不甘心的咸鱼注*/
	/*************************************************************************************************************/
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    
	/*电机驱动初始化*/
    /*自动重装值是1000，分完频再结合arr得到PWM频率1kHz，*/
	TIM1_dead_pwm_init(1000-1, 72-1, 0, 100);
    motor_init();
	
	/* 编码器初始化（TIM2）不分频，72MHZ的计数频率,ARR为最大值*/
    TIM2_encode_init(0xFFFF, 0);
	
    
    /*PID控制器*/
    extern PID_TypeDef g_speedPID;
    //PID_Init(&g_speedPID, 15.0f, 1.7f, 0.5f, 1000.0f, 0.0f); /* 初始化PID控制器，设置Kp、Ki、Kd以及输出限幅 */
	PID_Init(&g_speedPID, 14.5f, 1.65f, 1.0f, 1000.0f, 0.0f);/*	 */

    /* 延时函数初始化 */
    delay_init();

    /* 初始化蜂鸣器 */
	Beep_Init(&bep,GPIOB,GPIO_Pin_15);		
    /* 初始化按键模块 */
    Key_Init();
    MQ2_Init();     /* 初始化MQ2传感器 */
    WindSpeed_Init(); /* 初始化风速算法数据结构 */


    /* 初始化 LCD 显示模块，必须要进行添加 */
    LCD_Init();
    /* 清屏 */
    LCD_Clear(WHITE); 

    /* 串口初始化 */
    uart_init(115200);		/*必须放在后边初始化，否则会影响电机转动*/
}

int main(void)
{
	Hardware_Init();	/* 硬件初始化 */

    System_Init();		

    /* 创建开始任务 */
    StartTask_Create();
    
    /* 启动FreeRTOS调度器 */
    vTaskStartScheduler();


	while(1)
	{
		// //SpeedCalcTask();	/* 执行速度计算任务 */
		// UIDisplayTask();		/* 执行UI显示任务 */

        // //  /* PID控制电机速度 */
        // // PID_SetTarget(&motor_pid, 190.0f); /* 设置目标转速为150 RPM */
        // // pid_output = PID_Calculate(&motor_pid, speed); /* 计算PID输出值 */
        // // motor_pwm_set(pid_output); /* 根据PID输出值调整电机PWM占空比 */
        // // //motor_pwm_set(200);

        // KeyScanTask();			/* 执行按键扫描任务 */
		
        
	}
}
