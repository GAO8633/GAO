#include "mq2.h"
#include <math.h> 

/**
 * @brief 初始化MQ2传感器（ADC）
 */
void MQ2_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    
    /* 使能GPIO和ADC1时钟 */
    RCC_APB2PeriphClockCmd(MQ2_ADC_GPIO_CLK | RCC_APB2Periph_ADC1, ENABLE);
    
    /* 设置ADC分频因子 72M/6=12M，ADC最大时钟不能超过14M */
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    
    /* 配置PA4为模拟输入 */
    GPIO_InitStructure.GPIO_Pin = MQ2_ADC_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;   /* 模拟输入 */
    GPIO_Init(MQ2_ADC_GPIO_PORT, &GPIO_InitStructure);
    
    /* 复位ADC1 */
    ADC_DeInit(ADC1);
    
    /* ADC1配置 */
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;              /* 独立模式 */
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;                   /* 单通道模式 */
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;             /* 单次转换模式 */
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; /* 软件触发 */
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;          /* 右对齐 */
    ADC_InitStructure.ADC_NbrOfChannel = 1;                         /* 1个转换通道 */
    ADC_Init(ADC1, &ADC_InitStructure);
    
    /* 使能ADC1 */
    ADC_Cmd(ADC1, ENABLE);
    
    /* ADC校准 */
    ADC_ResetCalibration(ADC1);                     /* 复位校准寄存器 */
    while (ADC_GetResetCalibrationStatus(ADC1));    /* 等待复位完成 */
    ADC_StartCalibration(ADC1);                     /* 开始校准 */
    while (ADC_GetCalibrationStatus(ADC1));         /* 等待校准完成 */
    
    /* MQ2需要预热时间，但不在此处等待 */
}

/**
 * @brief 获取ADC原始值
 * @return ADC转换值（0-4095）
 */
uint16_t MQ2_GetAdcValue(void)
{
    u16 adc_value;
    
    /* 配置ADC1通道4，采样时间239.5个周期 */
    ADC_RegularChannelConfig(ADC1, MQ2_ADC_CHANNEL, 1, ADC_SampleTime_239Cycles5);
    
    /* 启动ADC转换，开启了软件触发，所以需要软件再开启 */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    
    /* 等待转换完成 */
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    
    /* 获取转换结果 */
    adc_value = ADC_GetConversionValue(ADC1);
    
    return adc_value;

}

/**
 * @brief 获取气体浓度值（多次采样取平均）
 * @return 气体浓度值，范围0-500
 * @note 将ADC值(0-4095)映射到气体浓度(0-500)
 */
float MQ2_GetGasConcentration(void)
{
    u8 i;
    u32 sum = 0;
    u16 adc_avg;
	float tmep,RS;
    float gas_concentration;
    
    /* 采样10次取平均值 */
    for (i = 0; i < 10; i++)
    {
        sum += MQ2_GetAdcValue();
        delay_us(100);
    }
    adc_avg = sum / 10;
    
	
	/*获取模拟电压值*/
    tmep = (float)adc_avg * (3.3/4096);
	
	/*通过模拟电压值计算气体浓度，公式如下所示*/
	RS=(5 - tmep)/tmep * 0.5;
	
    gas_concentration = pow(11.5428*2/RS,0.6549f)*100;        
	
    return gas_concentration;
}

/**
 * @brief 检测气体浓度是否超过阈值
 * @param threshold: 浓度阈值
 * @return 1:超过阈值; 0:未超过阈值
 */
u8 MQ2_IsGasDetected(u16 threshold)
{
    u16 gas_value = MQ2_GetGasConcentration();
    
    if (gas_value > threshold)
    {
        return 1;
    }
    return 0;
}
