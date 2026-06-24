#ifndef __PID_H
#define __PID_H

#include "sys.h"

typedef struct
{
    float Kp;           /* 比例系数 */
    float Ki;           /* 积分系数 */
    float Kd;           /* 微分系数 */
    
    float target;       /* 目标值（设定值） */
    float actual;       /* 实际值（反馈值） */
    
    float error;        /* 当前误差 */
    float last_error;   /* 上次误差 */
    float integral;     /* 误差积分累加值 */
    
    float output;       /* PID输出值 */
    float output_max;   /* 输出上限 */
    float output_min;   /* 输出下限 */
    
    float integral_max; /* 积分上限（防止积分饱和） */
} PID_TypeDef;

/* init初始化，settarget设置目标转速，calculate计算PID输出，要配合motor输出函数传入pid输出值。Reset清零 */
void PID_Init(PID_TypeDef *pid, float Kp, float Ki, float Kd, float out_max, float out_min);
void PID_SetTarget(PID_TypeDef *pid, float target);
float PID_Calculate(PID_TypeDef *pid, float actual);
void PID_Reset(PID_TypeDef *pid);




#endif
