#ifndef __DELAY_H
#define __DELAY_H

#include "sys.h"

void delay_init(void);
void delay_ms(u32 nms);
void delay_us(u32 nus);
void delay_xms(u32 nms);

#define Delay_init(void) delay_init(void)
#define Delay_ms(nms) delay_ms(nms)
#define Delay_us(nus) delay_us(nus)
#define Delay_xms(nms) delay_xms(nms)


#endif
