#ifndef __DHT11_H
#define __DHT11_H

#include "sys.h"
#include "stm32f10x.h"                  // Device header
#include "gpio.h"
#include "Delay.h"

/* DHT11不需要初始化，ReadData会进行start */
	 
 void DHT11_Start(led_d *io);
 void DHT11_Read(led_d *io);
 u8 DHT_Read_Byte(led_d *io);
 u8 DHT_Read_Data(u8 *temp,u8 *humi,gpioled port,u16 pin,led_d *io);
 void Clock(led_d *io);
 u8 readpin(led_d *io);


#endif
