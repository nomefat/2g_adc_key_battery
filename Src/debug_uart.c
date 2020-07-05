#include "main.h"
#include "stm32f1xx_hal.h"
#include "debug_uart.h"
#include "string.h"
#include "math.h"
#include "gprs_app.h"


extern UART_HandleTypeDef huart1;




char debug_uart_dma_buff[Q_LEN];       //队列数组

char debug_uart_buff[Q_LEN+1];

char debug_send_buff[Q_LEN];


struct_debug_double_buff debug_send_buff1;
struct_debug_double_buff debug_send_buff2;





struct _cmd_param_int{
	int param_num;
	int param[10];
}cmd_param_int;


struct _cmd_list{
	char *cmd;
	void (*func)(char *param);
};

#define CMD_CALLBACK_LIST_BEGIN struct _cmd_list cmd_list[] = {NULL,NULL,
#define CMD_CALLBACK_LIST_END NULL,NULL};
#define CMD_CALLBACK(cmd_string,callback)	cmd_string,callback,


int copy_string_to_double_buff(const char *pstr)
{
	
	int len = strlen(pstr);
	
	if(debug_send_buff1.len != 0xffffffff)
	{
		if(len<(DEBUG_DOUBLE_BUFF_LEN-debug_send_buff1.len))
		{			
			memcpy(debug_send_buff1.data+debug_send_buff1.len,pstr,len);
			debug_send_buff1.len += len;
			
			return 0;
		}	
	}
	
	if(debug_send_buff2.len != 0xffffffff)
	{
		if(len<(DEBUG_DOUBLE_BUFF_LEN-debug_send_buff2.len))
		{			
			memcpy(debug_send_buff2.data+debug_send_buff2.len,pstr,len);
			debug_send_buff2.len += len;
			return 0;
		}	
	}
	return -1;
}



/*****************
有main函数调用 双缓冲有数据就启动DMA发送
******************/
void debug_send_double_buff_poll(void)
{
	static int len = 0,i;

	if(huart1.gState == HAL_UART_STATE_READY)
	{
		if(debug_send_buff1.len == 0xffffffff)
			debug_send_buff1.len = 0;
		else if(debug_send_buff2.len == 0xffffffff)
			debug_send_buff2.len = 0;
	}
	else
		return;
				
	//缓冲中有数据需要发送  2个缓冲并没有正在发送
	if(debug_send_buff1.len != 0 && debug_send_buff1.len != 0xffffffff && huart1.gState == HAL_UART_STATE_READY)
	{		
		
		len = debug_send_buff1.len;
		debug_send_buff1.len = 0xffffffff;  //表明这个缓冲正在发送，不能操作
		HAL_UART_Transmit_DMA(&huart1,(uint8_t *)debug_send_buff1.data,len);		
	}
	else if(debug_send_buff2.len != 0 && debug_send_buff2.len != 0xffffffff && huart1.gState == HAL_UART_STATE_READY)
	{		
		len = debug_send_buff2.len;
		debug_send_buff2.len = 0xffffffff;
		HAL_UART_Transmit_DMA(&huart1,(uint8_t *)debug_send_buff2.data,len);		
	}
}




/**********************************************************************************************
***func: 用于单片第一次开启DMA接收
***     
***date: 2017/6/9
*** nome
***********************************************************************************************/
void start_from_debug_dma_receive(void)
{
	SET_BIT((&huart1)->Instance->CR1, USART_CR1_IDLEIE);  //打开串口空闲中断
	HAL_UART_Receive_DMA(&huart1,(uint8_t *)debug_uart_dma_buff,Q_LEN);	 //打开DMA接收
}




/**********************************************************************************************
***func:串口空闲中断回调函数
***     空闲中断用来判断一包数据的结束
***date: 2017/6/9
*** nome
***********************************************************************************************/
void uart_from_debug_idle_callback(void)
{
	HAL_DMA_Abort((&huart1)->hdmarx);
	huart1.RxState = HAL_UART_STATE_READY;
	huart1.hdmarx->State = HAL_DMA_STATE_READY;
	debug_uart_buff[0] = Q_LEN - huart1.hdmarx->Instance->CNDTR; 
	memcpy(debug_uart_buff+1,(char*)debug_uart_dma_buff,Q_LEN - huart1.hdmarx->Instance->CNDTR);
	
	HAL_UART_Receive_DMA(&huart1,(uint8_t *)debug_uart_dma_buff,Q_LEN);	 //打开DMA接收

}


/*
 * 功能：从队列中取出一个完整的字符串命令。
 * 失败：返回-1
 * 成功：返回0
 *cmd 存放命令的指针，param 存放参数的指针。
*/
int get_q_string(char *cmd,char *param)
{
	int i = 1;
	int timeout = 0;


	for(;;){
		if(debug_uart_buff[i] == ' ')
		{
			cmd[i-1] = 0; 
			i++;
			break;
		}
		else if(debug_uart_buff[i] == '\r' || debug_uart_buff[i] == '\n')
		{
			debug_uart_buff[0] = 0;
			cmd[i-1] = 0; 
			return 0;
		}
		if(i>debug_uart_buff[0])
			return -1;
		cmd[i-1] = debug_uart_buff[i];
		i++;
				
	}

	for(;;){

		if(debug_uart_buff[i] == '\r' || debug_uart_buff[i] == '\n')
		{
			debug_uart_buff[0] = 0;
			*param = 0; 
			return 0;
		}
		if(i>debug_uart_buff[0])
			return -1;	
		
		*param++ = debug_uart_buff[i];
		i++;	
	}

	return 0;
}

int str_to_int(char *str)
{
	int i = 0,j = 0;
	int ret = 0;
	for(;;){
	if(str[i++]==0||i>20)
		break;
	}
	j = i = i-2;
	for(;i>=0;i--)
	{
		ret += (str[i]-'0')*(pow(10,(j-i)));
	}
	return ret;
}

//
struct _cmd_param_int* get_str_param(char *param)
{
	char *ptr_now = param;
	char *ptr_pre = param;
	int i = 0;
	
	cmd_param_int.param_num = 0;
	for(;;){                     //分割参数，按照空格

		if(*ptr_now==' ')
		{
			*ptr_now = 0;
			cmd_param_int.param[i++] = str_to_int(ptr_pre);
			ptr_pre = ptr_now+1;
			cmd_param_int.param_num++;
		}
		ptr_now++;		
		if(*ptr_now==0)
		{
			cmd_param_int.param[i] = str_to_int(ptr_pre);
			cmd_param_int.param_num++;
			return &cmd_param_int;
		}
		if(ptr_now-param>100)		
			return &cmd_param_int;
	}
	
}

void sys_print(void)
{
	if(wake_up_who == WAKE_UP_KEY)
		copy_string_to_double_buff("Sensor_V2.0 start Key\r\n");
	else if(wake_up_who == WAKE_UP_POWER)
		copy_string_to_double_buff("Sensor_V2.0 start Power\r\n");
	else if(wake_up_who == WAKE_UP_RTC_ALARM)
		copy_string_to_double_buff("Sensor_V2.0 start Rtc Alarm\r\n");	
}

void help(char *param)
{
	copy_string_to_double_buff("*******************************************************\r\n\
Sensor_V2.0 cmd help\r\n"
);

	if(wake_up_who == WAKE_UP_KEY)
		copy_string_to_double_buff("wake up Key\r\n");
	else if(wake_up_who == WAKE_UP_POWER)
		copy_string_to_double_buff("wake up Power\r\n");
	else if(wake_up_who == WAKE_UP_RTC_ALARM)
		copy_string_to_double_buff("wake up Rtc Alarm\r\n");	

}

void cmd_gprs_send(char *param)
{
	task_send_gprs_data_enable = 1;
	
	copy_string_to_double_buff("start gprs send \r\n");
	
}

//在此处添加你的命令字符串和回调函数
CMD_CALLBACK_LIST_BEGIN

CMD_CALLBACK("?",help)		
CMD_CALLBACK("gprs_send",cmd_gprs_send)

CMD_CALLBACK_LIST_END









char cmd[100];
char param[32];
int get_cmd(void)
{
	int i = 0;
	if(get_q_string(cmd,param) == -1)
		return 0;
	
	for(;;){
		if(strcmp(cmd,cmd_list[i].cmd)==0)
			return i;	
		if(cmd_list[++i].cmd==NULL)
			return 0;
	}
}



void debug_cmd_handle(void)
{
	int func_index = get_cmd();
	if(func_index<=0)
		return;
	cmd_list[func_index].func(param);
}



void task_debug(void)
{
	debug_cmd_handle();
	
	debug_send_double_buff_poll();
	
}










