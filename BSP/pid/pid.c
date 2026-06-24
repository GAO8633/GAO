#include "pid.h"


void PID_Init(PID_TypeDef *pid, float Kp, float Ki, float Kd, float out_max, float out_min)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    
    pid->target = 0.0f;
    pid->actual = 0.0f;
    
    pid->error = 0.0f;
    pid->last_error = 0.0f;
    pid->integral = 0.0f;
    
    pid->output = 0.0f;
    pid->output_max = out_max;
    pid->output_min = out_min;
    
    /* 积分限幅设为输出限幅的一半，防止积分饱和 */
    pid->integral_max = out_max / 2.0f;
}

void PID_SetTarget(PID_TypeDef *pid, float target)
{
    pid->target = target;
}


/**
 * @brief PID计算输出值（位置式）
 * @note 位置式PID公式：output = Kp*e(k) + Ki*Σe(k) + Kd*[e(k)-e(k-1)]
 * @param pid: PID结构体指针
 * @param actual: 当前实际值
 * @return PID输出值
 */
float PID_Calculate(PID_TypeDef *pid, float actual)
{
    pid->actual= actual;

    pid->error = pid->target - pid->actual;  /* 计算当前误差 */
    pid->last_error = pid->error;            /* 更新上次误差 */

    pid->integral += pid->error;             /* 误差积分累加 */

    /* 积分限幅，防止积分饱和，有正饱和和负饱和 */
    if (pid->integral > pid->integral_max)
    {
        pid->integral = pid->integral_max;
    }
    else if (pid->integral < -pid->integral_max)
    {
        pid->integral = -pid->integral_max;
    }

    /*开始计算PID输出*/
    float p_out, i_out, d_out;
    p_out = pid->Kp * pid->error;           /* 比例项 */
    i_out = pid->Ki * pid->integral;        /* 积分项 */
    d_out = pid->Kd * (pid->error - pid->last_error); /* 微分项 */

    pid->output = p_out + i_out + d_out; /* PID输出 */

    /* 输出限幅 */
    if (pid->output > pid->output_max)
    {
        pid->output = pid->output_max;
    }
    else if (pid->output < pid->output_min)
    {
        pid->output = pid->output_min;
    }

    return pid->output;

}

/**
 * @brief 复位PID控制器
 * @param pid: PID结构体指针
 */
void PID_Reset(PID_TypeDef *pid)
{
    pid->error = 0.0f;
    pid->last_error = 0.0f;
    pid->integral = 0.0f;
    pid->output = 0.0f;
}


