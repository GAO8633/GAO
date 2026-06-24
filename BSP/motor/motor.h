#ifndef __MOTOR_H
#define __MOTOR_H
#include "sys.h"



#include "gpio.h"

/*扩展作用域*/
extern volatile float speed;                /* 实际转速 */
extern volatile int overflow;               /* 编码器溢出计数器 */

/*电机转向枚举类型*/
typedef enum{
	stright=0,
	invert
}direction;

/*	电机初始化只需要调用TIM1_dead_pwm_init()
*	然后再调用motor_init()函数，motor_init函数会调用motor_dir、motor_stop、motor_start函数来判断电机是否正确配置
*	电机速度控制只需要调用motor_pwm_set()函数，输入正数为正转，输入负数为反转，输入绝对值为占空比，范围为0-1000
*	
*/

/*********函数声明*************************/
void io_set(gpioled port,u16 pin,GPIOMode_TypeDef mode);
void TIM1_dead_pwm_init(u16 arr,u16 psc,u16 ccr,u16 dtg);
void motor_stop(void);
void motor_start(void);
void motor_dir(direction para);
void motor_speed(u16 ccr);
void TIM2_encode_init(u16 arr, u16 psc);
void TIM4_init(u16 arr,u16 psc);
int get_encoder_value(void);
u8 TIM_GetDirection(TIM_TypeDef* TIMx);
float get_speed(int encode_value,u16 ms);
void motor_pwm_set(float para);
void motor_init(void);
/**********函数声明**************************/

#endif
