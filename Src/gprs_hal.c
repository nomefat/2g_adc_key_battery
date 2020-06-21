#include "stm32f1xx_hal.h"
#include "string.h"
#include "gprs_hal.h"
#include "main.h"




#define GPRS_RCV_DMA_BUFF_LENGTH (1024+512)  // gprs串口接收


extern UART_HandleTypeDef huart3;

extern void gprs_str_copy_to_queue(unsigned short len,char* p_data);




uint8_t gprs_receive_dma_buff[GPRS_RCV_DMA_BUFF_LENGTH];






/**********************************************************************************************
***func: 用于单片第一次开启DMA接收
***     
***date: 2017/6/9
*** nome
***********************************************************************************************/
void start_from_gprs_dma_receive()
{
	SET_BIT((&huart3)->Instance->CR1, USART_CR1_IDLEIE);  //打开串口空闲中断
	HAL_UART_Receive_DMA(&huart3,gprs_receive_dma_buff,GPRS_RCV_DMA_BUFF_LENGTH);	 //打开DMA接收
}




/**********************************************************************************************
***func:串口空闲中断回调函数
***     空闲中断用来判断一包数据的结束
***date: 2017/6/9
*** nome
***********************************************************************************************/
void uart_from_gprs_idle_callback(void)
{
	HAL_DMA_Abort((&huart3)->hdmarx);
	huart3.RxState = HAL_UART_STATE_READY;
	huart3.hdmarx->State = HAL_DMA_STATE_READY;
	gprs_str_copy_to_queue(GPRS_RCV_DMA_BUFF_LENGTH-huart3.hdmarx->Instance->CNDTR,(char*)gprs_receive_dma_buff);

	HAL_UART_Receive_DMA(&huart3,gprs_receive_dma_buff,GPRS_RCV_DMA_BUFF_LENGTH);	 //打开DMA接收


	
}



void grps_power_off()
{
	HAL_GPIO_WritePin(M26_PWR_CTRL_GPIO_Port,M26_PWR_CTRL_Pin,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(M26_PWR_KEY_GPIO_Port,M26_PWR_KEY_Pin,GPIO_PIN_RESET);	
}


void grps_power_on()
{
	HAL_GPIO_WritePin(M26_PWR_CTRL_GPIO_Port,M26_PWR_CTRL_Pin,GPIO_PIN_SET);
	
	HAL_GPIO_WritePin(M26_PWR_KEY_GPIO_Port,M26_PWR_KEY_Pin,GPIO_PIN_SET);	

}



void gprs_uart_send_string(char *pstr)
{
	int num = 0;
	char *ptr = pstr;
	while(*ptr++)
	{
		num++;
		if(num>5000)
			return;
	}
	
	HAL_UART_Transmit_DMA(&huart3,(uint8_t *)pstr,num);
}


void gprs_uart_send_bin(void *pdata,uint16_t len)
{
	HAL_UART_Transmit_DMA(&huart3,(uint8_t *)pdata,len);
}


void led_ctrl(uint8_t led,uint8_t stat)
{
	if(led == LED_R)
	{
		if(stat == LED_ON)
			HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_SET);
		else
			HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_RESET);
	}
	else 	if(led == LED_G)
	{
		if(stat == LED_ON)
			HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_SET);
		else
			HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_RESET);
	}
	else 	if(led == LED_B)
	{
		if(stat == LED_ON)
			HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_SET);
		else
			HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_RESET);
	}
	
}

void led_flash(uint8_t led )
{
	if(LED_R)
	{
		HAL_GPIO_TogglePin(LED_R_GPIO_Port,LED_R_Pin);
	}
	else 	if(LED_G)
	{
		HAL_GPIO_TogglePin(LED_G_GPIO_Port,LED_G_Pin);
	}
	else 	if(LED_B)
	{
		HAL_GPIO_TogglePin(LED_B_GPIO_Port,LED_B_Pin);
	}
	
}








