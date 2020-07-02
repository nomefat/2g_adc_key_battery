#include "stm32f1xx_hal.h"
#include "string.h"
#include "math.h"
#include "main.h"
#include "gprs_hal.h"
#include "gprs_app.h"
#include "gprs_comm.h"
#include "rtc.h"
#include "debug_uart.h"
#include "stdio.h"
#include "communication_protocol_handle.h"



#define debug(str) copy_string_to_double_buff(str)


int gprs_data_queue_write[CLIENT_COUNT] = {0};               
int gprs_data_queue_read[CLIENT_COUNT] = {0};

uint8_t gprs_data_buff_client[CLIENT_COUNT][GPRS_DATA_BUFF_LENGH]; 

uint8_t task_send_gprs_data_enable = 0;

uint8_t btn_event = 0;

uint8_t wake_up_who = WAKE_UP_POWER;   //通过那个方式wakeup

struct_gprs_task_manage gprs_task_manage;



extern uint8_t rev_data_buf[];

extern uint8_t adc_data_buf[];

extern uint8_t send_data_buf[];

extern struct_protocol_adc_data protocol_adc_data;               //adc数据的协议,记录包总数和当前包数

extern struct_protocol_adc_data protocol_adc_data_s_get;               //adc数据的协议, 记录服务器索要的包序号

extern struct_adc_data_param adc_data_param;  //adc数据块的一些参数 


extern uint8_t bule_switch;  //蓝牙关闭


void btn_handle(void);

void task_2g_rev_data_handle(void);


/*
 * 
 * 
 * 
*/
int8_t gprs_data_write_queue(uint8_t c,uint8_t data)
{
    if(c>=CLIENT_COUNT)
        return -2;

	if((gprs_data_queue_write[c]+1)%GPRS_STR_QUEUE_LENGH==gprs_data_queue_read[c])
		return -1;

	gprs_data_buff_client[c][gprs_data_queue_write[c]] = data;
	gprs_data_queue_write[c] = (gprs_data_queue_write[c]+1)%GPRS_DATA_BUFF_LENGH;

	return 0;
}

/*
 * 
 * 
 * 
*/
int8_t gprs_data_read_queue(uint8_t c,uint8_t *pdata)
{
    if(c>=CLIENT_COUNT)
        return -2;  

	if(gprs_data_queue_write[c]==gprs_data_queue_read[c])
		return -1;	

	*pdata = gprs_data_buff_client[c][gprs_data_queue_read[c]];
	gprs_data_queue_read[c] = (gprs_data_queue_read[c]+1)%GPRS_STR_QUEUE_LENGH;
	return 0;
}




void task_gprs_app(void)
{
	
	task_send_gprs_data();	
	
	task_2g_rev_data_handle();

	btn_handle();
	
}




void task_send_gprs_data(void)
{
	static uint32_t ms = 0;
	static uint16_t len;
	int32_t ret;
	uint16_t crc;


	
	if(ms > get_ms_count())
		return;

	if(task_send_gprs_data_enable == 1)
	{
		if(gprs_send_data(0,"hello world",11,0) == 0)
			task_send_gprs_data_enable = 0;
	}
	else if(gprs_task_manage.send_adc_data_enable == 1)  //查询到有发送adc数据的任务
	{	
		if(gprs_task_manage.task_setp == TASK_NONE) //第一步 准备adc数据
		{
			if(make_adc_protocal_data() == 0)
			{
				gprs_task_manage.task_setp = SEND_ADC_DATA_ING;
			}
			else
			{
				//adc数据准备出错
				return;
			}
			
		}	
		if(gprs_task_manage.task_setp == SEND_ADC_DATA_ING)
		{
			if(protocol_adc_data.head.sid_cmd != DEV_2G_SEND_ADC_DATA || protocol_adc_data.packet_count == 0)
			{
				protocol_adc_data.head.sid_cmd = DEV_2G_SEND_ADC_DATA;
				protocol_adc_data.head.len = 4 + sizeof(adc_data_param) + 1024; 
				protocol_adc_data.head.len_xor = xor_fun((uint8_t *)&(protocol_adc_data.head.len),4); 
				if((adc_data_param.dev_sampling_rate*6)%1024 == 0)
					protocol_adc_data.packet_count =  (adc_data_param.dev_sampling_rate*6)/1024;
				else
				{
					protocol_adc_data.packet_count =  (adc_data_param.dev_sampling_rate*6)/1024 + 1;
				}
				protocol_adc_data.packet_index = 0;
				len = sizeof(protocol_adc_data);
				memcpy(send_data_buf,&protocol_adc_data,len);
				memcpy(&send_data_buf[len],&adc_data_param,sizeof(adc_data_param));
				len += sizeof(adc_data_param);
				memcpy(&send_data_buf[len],&adc_data_buf[protocol_adc_data.packet_index*1024],1024);
				len += 1024;
				crc = MB_CRC16(send_data_buf,len);
				send_data_buf[len++] = (crc & 0xff);
				send_data_buf[len++] = ((crc>>8) & 0xff);
			}
			if(len>0)
			{
				ret = gprs_send_data(0,send_data_buf,len,&gprs_task_manage.error_to_reboot_mod_times);
				if(ret == 99)  //进入休眠
				{

				}	
				else if(ret == -1)
				{
					ms = get_ms_count() + 1000;
				}	
				else if(ret == 0) //写入buf  可以继续准备写入下一条了
				{
					ms = get_ms_count() + 100;	
					if(gprs_task_manage.task_setp == SEND_ADC_DATA_ING)
					{
						protocol_adc_data.packet_index++;
						if(protocol_adc_data.packet_index >= protocol_adc_data.packet_count) //发送完成
						{
							gprs_task_manage.task_setp = WAIT_PROCOTAL_SEND_OK;
							memset(&protocol_adc_data,0,sizeof(protocol_adc_data));	
							len = 0;						
							return;
						}
						len = sizeof(protocol_adc_data);
						memcpy(send_data_buf,&protocol_adc_data,len);
						memcpy(&send_data_buf[len],&adc_data_buf[protocol_adc_data.packet_index*1024],1024);
						len += 1024;
						crc = MB_CRC16(send_data_buf,len);
						send_data_buf[len++] = (crc & 0xff);
						send_data_buf[len++] = ((crc>>8) & 0xff);
					}	
				}
				else if(ret == -3 || ret == -2)
				{
					ms = get_ms_count() + 100;				
				}	
			}
			else
			{

			}						
		}
		else if(gprs_task_manage.task_setp == WAIT_PROCOTAL_SEND_OK) //等待发送OK
		{

		}
	}
}



//处理链接0接收到的的数据
void task_2g_rev_data_handle(void)
{
	uint8_t data;
	static uint16_t index = 0;
	static uint8_t len_xor_flag = 0;
	uint16_t packet_crc;
	struct_protocol_head *p_protocol_head = (struct_protocol_head *)rev_data_buf;

	for(;;)
	{
		if(gprs_data_read_queue(0,&data) == 0)
		{
			rev_data_buf[index++] = data;
			if(index >=9)  //最小的无数据包长度是9
			{
				if(len_xor_flag == 0)  //len长度未校验
				{
					if(xor_fun((uint8_t *)&(p_protocol_head->len),4) != p_protocol_head->len_xor) //长度的校验出错 不是一个正确的包
					{
						index--;
						memcpy(rev_data_buf,&rev_data_buf[1],index);
						
					}
					else
					{
						len_xor_flag = 1;
					}
				}
				if(len_xor_flag == 1)  //第一步校验已经通过  等待进行crc校验
				{
					if(index >= (p_protocol_head->len+9)) //长度满足要求
					{
						packet_crc = (rev_data_buf[index-1]<<8) + rev_data_buf[index-2];
						if(packet_crc == MB_CRC16(rev_data_buf,p_protocol_head->len+7)) //校验通过
						{
							handle_2g_progocol(rev_data_buf);
						}
						else //校验失败
						{
							/* code */
						}						
						index = 0;
					}
				}	
			}
		}
		else
		{
			break;
		}
		
	}
}


void button_scan(void)
{
	 static uint32_t down_ms = 0;
	 static uint32_t up_ms = 0;	
	
	if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0) == GPIO_PIN_RESET) //松开
	{
		up_ms++;
		if(up_ms > 100) //判断为松开按键
		{
			if(down_ms > 2000) //长按键
			{
				btn_event = BTN_LONG_EVENT;
				down_ms = 0;		
			}
			else if(down_ms > 100) //短按键
			{
				if(wake_up_who == WAKE_UP_POWER)
				{
					wake_up_who = WAKE_UP_KEY;
					copy_string_to_double_buff("wake up from key\r\n");
				}
				else
				{
					led_ctrl(LED_R,LED_OFF);
				}
				btn_event = BTN_SHORT_EVENT;	
				down_ms = 0;			
			}
			up_ms = 0;	
		}
	}
	else  //按下
	{
		down_ms++;
		if(down_ms > 2000) //长按键
		{	
			led_ctrl(LED_G,LED_OFF);	
			led_ctrl(LED_R,LED_OFF);
			led_ctrl(LED_B,LED_OFF);			
		}
		else if(down_ms > 100) //短按键
		{
			up_ms = 0;	
			led_ctrl(LED_R,LED_ON);			
		}
	}
	
}





void btn_handle(void)
{
	if(btn_event == BTN_LONG_EVENT) //长按键
	{
		btn_event = 0;
		enter_standby();
	}
	else if(btn_event == BTN_SHORT_EVENT)  //短按键
	{
		btn_event = 0;
		if(get_ms_count() < 2000) //按键只是激活设备
		{
			
		}
		else    // 工作时候的按键 进行蓝牙开启或关闭
		{
			/* code */
			if(bule_switch)
			{
				bule_switch = 0;
				led_ctrl(LED_B,LED_OFF);
			}
			else
			{
				bule_switch = 1;
			}
			
		}
		
	}
	
}


void start_2g_send_adc_data_task(void)
{
	gprs_task_manage.send_adc_data_enable = 1;
	gprs_task_manage.task_setp = 0;
	gprs_task_manage.make_adc_error_times = 0;
	gprs_task_manage.error_to_reboot_mod_times = 300;
}

void wait_2g_adc_data_send_ok(void)
{
	if(gprs_task_manage.send_adc_data_enable == 1)
	{
		if(gprs_task_manage.task_setp == WAIT_PROCOTAL_SEND_OK)
		{
			gprs_task_manage.send_adc_data_enable = 2;
			gprs_task_manage.task_setp = 0;
			gprs_task_manage.make_adc_error_times = 0;
			gprs_task_manage.error_to_reboot_mod_times = 3;	
			sprintf(debug_send_buff,"4g_set:send adc data ok len=%d\r\n",adc_data_param.dev_sampling_rate*6);
			debug(debug_send_buff);				
		}
	}
}

int8_t get_2g_send_adc_data_stat(void)
{
	return gprs_task_manage.send_adc_data_enable;
}














