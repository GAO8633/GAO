#include "motor.h"
#include "gpio.h"

extern volatile int overflow;

 
/* 
* 调用的时候主要调用以下三个函数
* 死区时间PWM初始化函数。这个不仅初始换了TIM1还有对应PWM输出引脚和控制引脚
* 电机状态初始化，内部调用了start和stop函数进行初始化
* 电机完整控制函数，内部自动调用方向和速度控制函数，更具输入参数自动判断正反转以及速度控制
*/

/*
 * @brief: TIM1死区时间PWM初始化函数
 * @param: u16 arr - 自动重装载值
 * @param: u16 psc - 预分频值
 * @param: u16 ccr - 比较值(占空比)
 * @param: u16 dtg - 死区时间
 * @return: none
 */
void TIM1_dead_pwm_init(u16 arr,u16 psc,u16 ccr,u16 dtg)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	TIM_BDTRInitTypeDef TIM_BDTRInitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE); // 使能TIM1时钟
    //开始使能对应输出引脚和互补引脚
    io_set(GPIOA,GPIO_Pin_8,GPIO_Mode_AF_PP);   //配置成复用推挽输出
    //!!!!!!!!!!!!!!!!!!!配置的是GPIOB不是A！！！！！！！好好看参数，不要盲目用AI补全
    io_set(GPIOB,GPIO_Pin_13,GPIO_Mode_AF_PP); // 对应互补引脚，对应能实现该功能的引脚已固定不可改变
    //用来驱动电机驱动板是否开始使能，开关电机
    io_set(GPIOA,GPIO_Pin_2,GPIO_Mode_Out_PP);   // 配置成普通推挽输出，作为死区时间的控制引脚

    // 初始化TIM1时基单元
    TIM_TimeBaseStructure.TIM_Period = arr; // 设置自动重装载值
    TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV4; // 设置时钟分割
    TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; // 向上计数模式
    TIM_TimeBaseStructure.TIM_Prescaler=psc; // 设置预分频值
    //TIM_TimeBaseStructure.TIM_RepetitionCounter;   //多少个PWM周期更新一次中断，不赋值为0，即每个PWM周期更新一次中断
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure); // 初始化TIM1时基单元

    // 初始化TIM1输出比较单元
    //TIM_OCInitStructure.TIM_OCIdleState;  // 输出比较空闲状态（空闲即刹车）,如果是默认则为reset低电平，OCN也为低电平
    //TIM_OCInitStructure.TIM_OCNIdleState=;
    TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1; // 设置为PWM模式1
    TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High; // 输出比较极性为高
    TIM_OCInitStructure.TIM_OCNPolarity=TIM_OCNPolarity_High; // 互补输出比较极性为高
    TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable; // 使能输出比较
    TIM_OCInitStructure.TIM_OutputNState=TIM_OutputState_Enable; // 使能互补输出比较
    TIM_OCInitStructure.TIM_Pulse=ccr; // 设置比较值(占空比)
    TIM_OC1Init(TIM1, &TIM_OCInitStructure); // 初始化TIM1
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable); // 使能TIM1在CCR上的预装载寄存器


    //！！！！！！！不要完全让AI来写，AI自己补全是有问题的，要自己去看对应结构体的配置是怎么样的。
    /* 配置BDTR寄存器 */
    TIM_BDTRStructInit(&TIM_BDTRInitStructure);		
    TIM_BDTRInitStructure.TIM_AutomaticOutput=TIM_AutomaticOutput_Enable; // 使能自动输出
    TIM_BDTRInitStructure.TIM_Break=TIM_Break_Disable; // 使能刹车功能
    TIM_BDTRInitStructure.TIM_BreakPolarity=TIM_BreakPolarity_Low; // 刹车输入极性为高
    TIM_BDTRInitStructure.TIM_DeadTime=dtg; // 设置死区时间
    //TIM_BDTRInitStructure.TIM_LOCKLevel=; //这个是锁定级别，默认不锁定，不进行修改
    TIM_BDTRInitStructure.TIM_OSSRState=TIM_OSSRState_Enable; // 使能运行状态下的输出
    TIM_BDTRInitStructure.TIM_OSSIState=TIM_OSSIState_Disable; // 使能空闲状态输出,空闲状态不输出。
    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure); // 配置TIM1的BDTR寄存器
    TIM_CtrlPWMOutputs(TIM1, ENABLE);											// 使能MOE位
	TIM_Cmd(TIM1, ENABLE);  													// 使能TIM1

}


/*电机开启*/
void motor_start()
{
    io_set_bit(GPIOA,GPIO_Pin_2);		//使能驱动H桥电路
}


/*电机停止*/
void motor_stop()
{
    TIM_CCxCmd(TIM1,TIM_Channel_1,TIM_CCx_Disable);		// 关闭CH1通道PWM输出
	TIM_CCxNCmd(TIM1,TIM_Channel_1,TIM_CCxN_Disable); 	// 关闭CH1N通道PWM输出
	io_reset_bit(GPIOA,GPIO_Pin_2);						//禁止驱动H桥电路
}


/*电机方向控制*/
void  motor_dir(direction para)
{
    TIM_CCxCmd(TIM1,TIM_Channel_1,TIM_CCx_Disable);		// 关闭CH1通道PWM输出
    TIM_CCxNCmd(TIM1,TIM_Channel_1,TIM_CCxN_Disable); 	// 关闭CH1N通道PWM输出
    if(para==stright)
    {
        TIM_CCxNCmd(TIM1,TIM_Channel_1,TIM_CCxN_Enable); 	// 开启CH1N通道PWM输出
    }
    else if(para==invert)
    {
        TIM_CCxCmd(TIM1,TIM_Channel_1,TIM_CCx_Enable);		// 开启CH1通道PWM输出
    }

}


/*电机状态初始化，调用开启停止以及方向控制，判断电机是否正确配置*/
void motor_init(void)
{
	motor_dir(stright);
	motor_stop();
	motor_start();                                                                                
}


/*电机速度控制*/
void motor_speed(u16 ccr)
{
	if(ccr<=1000)	/* 限幅 */
	{
		TIM_SetCompare1(TIM1,ccr);	// 设置占空比
	}
}

/*电机完整控制，输入速度参数，自动判断正反转以及速度控制*/
void motor_pwm_set(float para)
{
    int val = (int)para;

    if (val >= 0) 
    {
        motor_dir(stright);           /* 正转 */
        motor_speed(val);
    } 
    else 
    {
        motor_dir(invert);           /* 反转 */
        motor_speed(-val);
    }
}


/*******     进行编码器配置       ******/
/* TIM2编码器功能初始化 */
void TIM2_encode_init(u16 arr, u16 psc)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef  TIM_ICInitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); // 使能TIM2时钟
    io_set(GPIOA, GPIO_Pin_0, GPIO_Mode_IPD); // 配置PA0为下拉输入，TIM2_CH1
    io_set(GPIOA, GPIO_Pin_1, GPIO_Mode_IPD); // 配置PA1为下拉输入，TIM2_CH2

    // 初始化TIM2时基单元
    TIM_TimeBaseStructure.TIM_Period = arr; // 设置自动重装载值
    TIM_TimeBaseStructure.TIM_Prescaler = psc; // 设置预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; // 时钟不分割
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    /*配置编码器通道CH1和CH2，进行计数。由于是AB两相都计数，所以四倍频*/
    TIM_ICInitStructure.TIM_Channel=TIM_Channel_1;
    TIM_ICInitStructure.TIM_ICFilter=10;
    TIM_ICInitStructure.TIM_ICPolarity=TIM_ICPolarity_Rising;	//上升沿捕获
    TIM_ICInitStructure.TIM_ICPrescaler=TIM_ICPSC_DIV1;			// 不分频，每一个边沿触发一次捕获;
    TIM_ICInitStructure.TIM_ICSelection=TIM_ICSelection_DirectTI;    // 直接连接到对应通道
    TIM_ICInit(TIM2, &TIM_ICInitStructure); // 初始化TIM2输入捕获

    TIM_ICInitStructure.TIM_Channel=TIM_Channel_2;
    TIM_ICInitStructure.TIM_ICFilter=10;
    TIM_ICInitStructure.TIM_ICPolarity=TIM_ICPolarity_Rising;	//上升沿捕获
    TIM_ICInitStructure.TIM_ICPrescaler=TIM_ICPSC_DIV1;			// 不分频，每一个边沿触发一次捕获;
    TIM_ICInitStructure.TIM_ICSelection=TIM_ICSelection_DirectTI;    // 直接连接到对应通道
    TIM_ICInit(TIM2, &TIM_ICInitStructure); // 初始化TIM2输入捕获

    // 配置TIM2编码器接口,配置编码器接口，使用TI1和TI2进行计数，捕获上升沿
    TIM_EncoderInterfaceConfig(TIM2, TIM_EncoderMode_TI12,TIM_ICPolarity_Rising,TIM_ICPolarity_Rising); 

    /* 配置TIM2中断，因为计数器为65536，可能会溢出所以配置中断 */
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  				// TIM2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;  		// 抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  			// 子优先级0级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 				// IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  								// 根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器

    TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE ); 						// 使能指定的TIM2中断，允许更新中断

    TIM_Cmd(TIM2, ENABLE); // 使能TIM2

}



/* 定时器Timer4初始化，用于计算转速 */
void TIM4_init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); 			// 时钟使能

	TIM_TimeBaseStructure.TIM_Period = arr; 						// 设置在下一个更新事件装入自动重装载寄存器周期的值	 
	TIM_TimeBaseStructure.TIM_Prescaler =psc; 						// 设置用来作为TIMx时钟频率除数的预分频值  
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; 					// 设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  	// TIM向上计数模式
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); 				// 根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位

	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE ); 						// 使能指定的TIM4中断，允许更新中断
	
	NVIC_InitStructure.NVIC_IRQChannel =TIM4_IRQn;  				// TIM4中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;  		// 抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  			// 子优先级0级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 				// IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  								// 根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器

	TIM_Cmd(TIM4, ENABLE);  										//使能定时器		

}

 //overflow是一个全局变量，在定时器中断函数中进行更新，表示溢出周期。一个周期代表65536个计数值。
/* 获取编码器计数值 */ 
int get_encoder_value(void)
{
	u32 buffer;
	buffer=TIM_GetCounter(TIM2)+(overflow*65536);
	return buffer;
}

/*
返回 1 = 计数器向下计数（反转）
返回 0 = 计数器向上计数（正转）
*/
/* 获取计数方向 */		//TIM是结构体地址，传入之后，可以访问其变量读取计数方向和计数值等信息
u8 TIM_GetDirection(TIM_TypeDef* TIMx)
{
    return (TIMx->CR1 & TIM_CR1_DIR) ? 1 : 0;	/*在编码器模式下，定时器的计数方向变为只读*/
}


/* 获取电机转速。ms=50表示每50ms计算一次速度，即采样精度为50ms*/
float get_speed(int encode_value,u16 ms)
{
	u8 i= 0;
//    u8 j= 0;
    float temp = 0.0;
	static float speed=0;									/*需要为静态变量，因为一阶滤波算法需要上次的滤波值 */
    static uint8_t sp_count = 0, k = 0;
    static float speed_arr[10] = {0.0};                     // 存储速度进行滤波数组 
	static int old_value=0,now_value=0;

    if (sp_count == ms)                                     // 计算一次速度 
    {
		now_value = encode_value;							// 记录当前编码器值
		
        // 计算转速，30为减速比，4倍频，11线
		
		/* 1000/ms指在这个ms时间内获得了xx脉冲变化值，用xx变化值/ms得到1ms的脉冲变化值
		 * 再乘以1000得到1s的变化值 */
		/*1000先不在意，先通过ms采样精度和30减速比以及四倍频十一线得到1ms的转速，然后通过1000和60得到1分钟的rpm*/
        speed_arr[k++] = (float)((now_value - old_value) * ((1000 / ms) * 60.0) / 30 / (11*4)); 
		old_value = now_value;								// 保存当前计数值
		
        /* 累计10次速度值，利用冒泡排序，后面做中值滤波 */
        /* gao更改算法：只得出最高最低值即可*/
		if (k == 10)
        {
			
            // for (i = 10; i >= 1; i--)                       
            // {
            //     for (j = 0; j < (i - 1); j++) 
            //     {
            //         if (speed_arr[j] > speed_arr[j + 1])    /* 数值比较 */
            //         { 
            //             temp = speed_arr[j];                /* 数值换位 */
            //             speed_arr[j] = speed_arr[j + 1];
            //             speed_arr[j + 1] = temp;
            //         }
            //     }
            // }
            
            // temp = 0.0;
            
            // for (i = 2; i < 8; i++)                         /* 去掉最高最低的数据 */
            // {
            //     temp += speed_arr[i];                       /* 将中间数值累加 */
            // }
            
            // temp = (float)(temp / 6);                       /* 求速度平均值 */
            
            /*gao改写*/
			float max = speed_arr[0];
			float min = speed_arr[0];
			float sum = 0;

			for(i=0;i<10;i++)
			{
				if(speed_arr[i] > max)
					max = speed_arr[i];

				if(speed_arr[i] < min)
					min = speed_arr[i];

				sum += speed_arr[i];
			}

            
            
			temp = (sum - max - min)/8.0f;



            /* 一阶低通滤波
             * 公式为：Y(n)= qX(n) + (1-q)Y(n-1)
             * 其中X(n)为本次采样值，Y(n-1)为上次滤波输出值，Y(n)为本次滤波输出值，q为滤波系数
             * q值越小，上一次输出对本次输出的影响越大，输出越平稳，但是对速度变化的响应也就越慢
             */
            speed = (float)( ((float)0.48 * temp) + (speed * (float)0.52) );
            k = 0;
        }
        sp_count = 0;
    }
    sp_count ++;
	return speed;
}


/* 注意：TIM2_IRQHandler和TIM4_IRQHandler已移至app_tasks.c中
 * 原因：中断服务程序需要访问FreeRTOS信号量，统一放在应用层文件中管理 */


