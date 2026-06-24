#ifndef __BEEP_H
#define __BEEP_H	 
#include "sys.h"
#include "gpio.h"

extern  led_d bep;


void Beep_Init(led_d *io,gpioled port,u16 pin);
void Beep_config(led_d *io);
void Beep_off(led_d *io);
void Beep_on(led_d *io);
void Buzzer_Beep(u16 duration_ms,led_d *io);
#endif
