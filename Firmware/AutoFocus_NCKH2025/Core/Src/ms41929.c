#include "ms41929.h"

extern SPI_HandleTypeDef hspi1;

// Gửi 24-bit: 8-bit Addr + 16-bit Data (Chuẩn LSB First)
void MS41929_WriteReg(uint8_t addr, uint16_t data) {
    uint8_t pData[3];
    pData[0] = (addr & 0x3F); // Header: 6-bit addr, R/W=0, C1=0
    pData[1] = (uint8_t)(data & 0xFF);
    pData[2] = (uint8_t)((data >> 8) & 0xFF);

    HAL_GPIO_WritePin(MS_CS_GPIO_Port, MS_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, pData, 3, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(MS_CS_GPIO_Port, MS_CS_Pin, GPIO_PIN_SET);
}

void MS41929_Init(void) {
    // Hardware Reset
    HAL_GPIO_WritePin(MS_RSTB_GPIO_Port, MS_RSTB_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(MS_RSTB_GPIO_Port, MS_RSTB_Pin, GPIO_PIN_SET);
    HAL_Delay(10);

    MS41929_WriteReg(MS_REG_PWM, 0x1E03);   // 112.5kHz PWM
    MS41929_WriteReg(MS_REG_PHASE_AB, 0x0002);
    MS41929_WriteReg(MS_REG_CUR_AB, 0x7878);   // 50% Current
    MS41929_WriteReg(MS_REG_STEP_AB, 0x040F);  // 256 Microstep, Enable
    MS41929_WriteReg(MS_REG_INTCT_AB, 0x1AB5); // Speed set
}

void MS41929_SetDirection(uint8_t dir) {
    // Bit 0 của Reg 2CH điều khiển hướng Motor A-B
    if(dir) MS41929_WriteReg(MS_REG_DIRECTION, 0x0001);
    else    MS41929_WriteReg(MS_REG_DIRECTION, 0x0000);
}

void MS41929_Step(uint16_t count) {
    for(uint16_t i=0; i<count; i++) {
        HAL_GPIO_WritePin(VD_FZ_GPIO_Port, VD_FZ_Pin, GPIO_PIN_SET);
        for(volatile int d=0; d<50; d++); // Pulse width ~10us
        HAL_GPIO_WritePin(VD_FZ_GPIO_Port, VD_FZ_Pin, GPIO_PIN_RESET);
        HAL_Delay(2); // Thời gian chờ giữa các bước (Tốc độ)
    }
}
