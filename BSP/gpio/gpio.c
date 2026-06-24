#include "gpio.h"





/* 封装GPIO操作函数，其中Pin参数可以是GPIO_Pin_0~GPIO_Pin_15的任意组合，
mode参数可以是GPIO_Mode_AIN、GPIO_Mode_IN_FLOATING、GPIO_Mode_IPD、GPIO_Mode_IPU、GPIO_Mode_Out_OD、GPIO_Mode_Out_PP、GPIO_Mode_AF_OD或GPIO_Mode_AF_PP中的任意一种
输入：GPIO_Mode_IN_FLOATING 浮空输入，GPIO_Mode_IPU 上拉输入，GPIO_Mode_IPD 下拉输入，
输出：GPIO_Mode_Out_OD 开漏输出，GPIO_Mode_Out_PP 推挽输出，GPIO_Mode_AF_OD 复用开漏输出，GPIO_Mode_AF_PP 复用推挽输出
GPIO_Speed 驱动PWM SPI等高速信号
*/
/* GPIO初始化函数 */
void io_set(gpioled port,u16 pin,GPIOMode_TypeDef mode)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	if(port==GPIOA) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); 
	else if(port==GPIOB) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	else if(port==GPIOC) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	else if(port==GPIOD) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	else if(port==GPIOE) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	else if(port==GPIOF) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
	else if(port==GPIOG) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = pin;  
	GPIO_InitStructure.GPIO_Mode = mode;  		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(port, &GPIO_InitStructure);			// 初始化GPIO
}

/* 设置GPIO引脚电平为高 */
void io_set_bit(gpioled port,u16 pin)
{
	GPIO_SetBits(port,pin);
}

/* 设置GPIO引脚电平为低 */
void io_reset_bit(gpioled port,u16 pin)
{
	GPIO_ResetBits(port,pin);
}
