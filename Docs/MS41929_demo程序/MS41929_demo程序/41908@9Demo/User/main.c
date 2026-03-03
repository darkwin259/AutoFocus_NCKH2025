/**
  ******************************************************************************
  * @file    
  * @author 
  * @version 
  * @date    
  * @brief  
  ******************************************************************************
  * @attention
  *
  * 
  * 
  * 
  * 
  ******************************************************************************
  */ 
#include "stm32f10x.h"
#include "bsp_usart1.h"
#include "bsp_SPI.h"
#include "bsp_41908.h"
#include "bsp_exti.h"
#include "bsp_TIMbase.h"
/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */
void Delay(int b)
{ 
	int a;
	for(a=0;a<b;a++)
	 for(a=0;a<b;a++);	
}
int main(void)
{
	SystemInit();
	USART1_Config();
  SPI_SPI2_Config();
	MS_Config();
	EXTI_IO_Config();
	TIM2_Configuration();
	
	printf("\r\n ***************************************\r\n");	
	printf("\r\n **********杭州瑞盟科技有限公司*********\r\n");	
  printf("\r\n ************MS41908@9Demo**************\r\n");	
	printf("\r\n 用于网络摄像机.监控摄像机用机头驱动芯片\r\n");	
	MS_Rest();//41908复位
	MS_Int1();
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 , ENABLE);
	while(1)
	{
  }
}
/*********************************************END OF FILE**********************/
