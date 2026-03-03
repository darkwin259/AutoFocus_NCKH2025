/****************************************************************************
* 
*
*          
* 
*  
*
* 
* 
*    
*
* 
* 
* 
*
*/

#include "stm32f10x.h"
#include "bsp_41908.h"
#include "stm32f10x_spi.h"
#include "bsp_SPI.h"

void MS_Config(void);
void MS_Rest(void);
void MS_Int1(void);
void Variable_A(unsigned int *aa,char *bb);
void Variable_B(unsigned int *cc,char *dd);
void LED_Change_A(char LED_addr,char *cc,unsigned int *dd);
//unsigned int Speed[10]={0xffff,0x2000,0x0aff,0x08ff,0x07ff,0x0600,0x0500,0x0300,0x0150,0x0100};
//char Circle_number[10]={0x01,0x07,0x17,0x1 c,0x20,0x2a,0x33,0x55,0xc2,0xff};

/*********************************************/
void MS_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/* config USART1 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	/*41908复位脚-PA8*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);    
}
/****************************************************************************
* 名    称：void MS_Rest(void)
* 功    能：MS41908复位
* 入口参数: 无
* 出口参数：无
* 说    明：41908_REST 高—低——高复位
* 调用方法：
****************************************************************************/  
void MS_Rest(void)
{
  MS_Hihg();
	__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();
	MS_Low();
	__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();
	MS_Hihg();	
}
/****************************************************************************
* 名    称：void MS_Int1(void)
* 功    能：MS41908初始化
* 入口参数: 无
* 出口参数：无
* 说    明：
* 调用方法：
****************************************************************************/  
void MS_Int1(void)
{

  Spi_Write(0x20,0x1e01);	 //设定PWM频率 DT1延时设为 0.3ms
	Spi_Write(0x22,0x0001);	 //DT2延时设为 0.3ms
	Spi_Write(0x23,0xd8d8);	 //设置AB通道最大占空比为 90%
	Spi_Write(0x24,0x0d20);	 //设置AB通道细分 电流方向 步数
	Spi_Write(0x25,0x07ff);  //设置AB通道步进周期
	Spi_Write(0x27,0x0002);	 //DT2延时设为 0.6ms
	Spi_Write(0x28,0xd8d8);	 //设置CD通道最大占空比为 90%
	Spi_Write(0x29,0x0d20);	 //设置CD通道细分 电流方向 步数
	Spi_Write(0x2a,0x07ff);  //设置CD通道步进周期
  Spi_Write(0x21,0x0087);  //把信号引导PLS1、PLS2  	
//  Spi_Write(0x00,0x0000);  //设置光圈目标值
//  Spi_Write(0x01,0x6000);  //设置ADC反馈滤波器截止频率 PID控制器数字增益
//  Spi_Write(0x02,0x66f0);  //设置PID控制器零点 极点
//  Spi_Write(0x03,0x0e10);  //设置光圈输出PWM频率
//  Spi_Write(0x04,0xd640);  //设置霍尔元件偏置电流 偏置电压
//  Spi_Write(0x05,0x0024);  //设置光圈目标值低通滤波器器截止频率
//  Spi_Write(0x0e,0x0300);  //设置光圈目标值移动平均速度
	Spi_Write(0x0b,0x0480);  //设置光圈模块使能 TESTEN1使能
	
}
/****************************************************************************
* 名    称：void Variable_A(unsigned int *aa,char *bb)
* 功    能：电机A变速
* 入口参数: 无
* 出口参数：无
* 说    明：
* 调用方法：
****************************************************************************/ 
void Variable_A(unsigned int *aa,char *bb)
{
	unsigned int addr24,addr25;
	addr24=(0x0500+*bb);
	addr25=*aa; 
	Spi_Write(0x24,addr24);
  Spi_Write(0x25,addr25);		
}
/****************************************************************************
* 名    称：void Variable_B(unsigned int *cc,char *dd)
* 功    能：电机B变速
* 入口参数: 无
* 出口参数：无
* 说    明：
* 调用方法：
****************************************************************************/ 
void Variable_B(unsigned int *cc,char *dd)
{
	unsigned int addr29,addr2a;
	addr29=(0x0500+*dd);
	addr2a=*cc; 
	Spi_Write(0x29,addr29);
  Spi_Write(0x2a,addr2a);	
}
/****************************************************************************
* 名    称: LED_Change_A(char LED_addr,char *cc,unsigned int *dd)
* 功    能：开关LED_A&B
* 入口参数: 无
* 出口参数：无
* 说    明：
* 调用方法：
****************************************************************************/ 
void LED_Change_A(char LED_addr,char *cc,unsigned int *dd)
{
	unsigned int LED_data;
	LED_data=(*dd+*cc);
	Spi_Write(LED_addr,LED_data);
}


