#ifndef __MQ2_H
#define __MQ2_H

#include "sys.h"
#include "stm32f10x.h"                  // Device header
#include "gpio.h"
#include "Delay.h"

/* MQ2 ADC引脚定义 */
#define MQ2_ADC_GPIO_PORT       GPIOA
#define MQ2_ADC_GPIO_PIN        GPIO_Pin_4
#define MQ2_ADC_GPIO_CLK        RCC_APB2Periph_GPIOA
#define MQ2_ADC_CHANNEL         ADC_Channel_4       /* ADC通道4 */

/* 气体浓度阈值定义（用于防回流模式） */
#define GAS_THRESHOLD_NORMAL    100.0f                  /* 正常阈值 */
#define GAS_THRESHOLD_HIGH      2000.0f                 /* 切换后的高阈值 */

/* 函数声明 */
void MQ2_Init(void);                                /* 初始化MQ2传感器 */
uint16_t MQ2_GetAdcValue(void);                          /* 获取ADC原始值 */
float MQ2_GetGasConcentration(void);                /* 获取气体浓度值（0-500） */
u8 MQ2_IsGasDetected(u16 threshold);                /* 检测是否超过阈值 */

#endif
