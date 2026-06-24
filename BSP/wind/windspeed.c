/*
 * 风速算法模块源文件
 * 
 * 算法说明：
 * 1. 传感器归一化：
 *    f_T = (T - T_base) / (T_max - T_base)
 *    f_H = (H - H_base) / (H_max - H_base)
 *     f_G = (G - G_base) / (G_max - G_base)
 * 
 * 2. 权重融合：
 *    F = w_t * f_T + w_h * f_H + w_g * f_G
 * 
 * 3. 映射到PWM：
 *    PWM = PWM_min + (PWM_max - PWM_min) * F
 * 
 */

#include "windspeed.h"

extern WindSpeed_t g_windSpeedData; /* 风速算法数据结构 */

static float Constrain(float value, float min, float max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void WindSpeed_Init(void)
{
    /* 初始化全局变量 */
    g_windSpeedData.f_T = 0.0f;
    g_windSpeedData.f_H = 0.0f;
    g_windSpeedData.f_G = 0.0f;
    g_windSpeedData.fusionValue = 0.0f;
    g_windSpeedData.pwmValue = 0.0f;
    g_windSpeedData.isCookingEvent = 0;
}

/**
 * @brief 更新传感器数据并计算风速
 * @param temp: 温度值 (℃)
 * @param humidity: 湿度值 (%)
 * @param gas: 气体浓度值 
 */
void WindSpeed_Update(u8 temp, u8 humidity, float gas)
{
    g_windSpeedData.f_T= (temp - TEMP_BASE) /(TEMP_MAX-TEMP_BASE);         /* 温度归一化，假设范围0-50℃ */
    g_windSpeedData.f_T=Constrain(g_windSpeedData.f_T, 0.0f, 1.0f); /* 确保归一化值在0-1范围内 */
    
    g_windSpeedData.f_H = (humidity - HUMIDITY_BASE) / (HUMIDITY_MAX-HUMIDITY_BASE);     /* 湿度归一化，假设范围0-100% */
    g_windSpeedData.f_H=Constrain(g_windSpeedData.f_H, 0.0f, 1.0f); /* 确保归一化值在0-1范围内 */
    
    g_windSpeedData.f_G = (gas-GAS_BASE) /(GAS_MAX- GAS_BASE);                /* 气体浓度归一化，假设范围0-500 */
    g_windSpeedData.f_G=Constrain(g_windSpeedData.f_G, 0.0f, 1.0f); /* 确保归一化值在0-1范围内 */

    /* 权重融合: F = w_t * f_T + w_h * f_H + w_g * f_G */
    g_windSpeedData.fusionValue = WEIGHT_TEMP * g_windSpeedData.f_T + 
                                  WEIGHT_HUMIDITY * g_windSpeedData.f_H + 
                                  WEIGHT_GAS * g_windSpeedData.f_G; 
    
    /* 映射到PWM: PWM = PWM_min + (PWM_max - PWM_min) * F */
    g_windSpeedData.pwmValue = PWM_MIN + (PWM_MAX - PWM_MIN) * g_windSpeedData.fusionValue;
    g_windSpeedData.pwmValue = Constrain(g_windSpeedData.pwmValue, PWM_MIN, PWM_MAX);
    
    /* 4. 判断是否为Cooking Event */
    if ((temp > COOKING_TEMP_THRESHOLD) &&
        ((humidity > COOKING_HUMIDITY_THRESHOLD) && (gas > COOKING_GAS_THRESHOLD)))
    {
        g_windSpeedData.isCookingEvent = 1;
    }
    else
    {
        g_windSpeedData.isCookingEvent = 0;
    }
}

/**
 * @brief 获取计算得到的PWM占空比
 * @return PWM占空比(1-100%)
 */
float WindSpeed_GetPWM(void)
{
    return g_windSpeedData.pwmValue;
}

/**
 * @brief 获取PWM占空比对应的CCR值
 * @param maxCompare: 定时器最大比较值（ARR值）
 * @return 定时器比较值
 */
u16 WindSpeed_GetPWMCCR(u16 maxCompare)
{
    return (u16)(g_windSpeedData.pwmValue * maxCompare / 100.0f);
}

/**
 * @brief 检测是否为Cooking Event
 * @return 1:是Cooking Event; 0:不是
 */
u8 WindSpeed_IsCookingEvent(void)
{
    return g_windSpeedData.isCookingEvent;
}

/**
 * @brief 获取风速数据结构体指针
 * @return 风速数据结构体指针
 */
WindSpeed_t* WindSpeed_GetData(void)
{
    return &g_windSpeedData;
}

/**
 * @brief 根据档位获取目标转速
 * @param level: 档位 (0:LOW, 1:HIGH)
 * @return 目标转速 (RPM)
 */
u16 WindSpeed_GetTargetRPM(u8 level)
{
    switch (level)
    {
        case 0:
            return SPEED_LOW_RPM;
        case 1:
            return SPEED_HIGH_RPM;
        default:
            return SPEED_LOW_RPM;
    }
}

u16 gao_WindSpeed_GetTargetRPM(u8 level)
{
    switch (level)
    {
        case 0:
            return gao_SPEED_LOW_RPM;
        case 1:
            return gao_SPEED_HIGH_RPM;
        default:
            return gao_SPEED_LOW_RPM;
    }
}
