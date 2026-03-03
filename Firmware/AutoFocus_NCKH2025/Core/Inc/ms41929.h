#ifndef __MS41929_H
#define __MS41929_H

#include "main.h"

// Địa chỉ thanh ghi (Datasheet MS41929)
#define MS_REG_PWM          0x20
#define MS_REG_PHASE_AB     0x22
#define MS_REG_CUR_AB       0x23
#define MS_REG_STEP_AB      0x24
#define MS_REG_INTCT_AB     0x25
#define MS_REG_DIRECTION    0x2C

void MS41929_Init(void);
void MS41929_WriteReg(uint8_t addr, uint16_t data);
void MS41929_SetDirection(uint8_t dir);
void MS41929_Step(uint16_t count);

#endif
