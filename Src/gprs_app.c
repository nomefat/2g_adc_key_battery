#include "stm32f1xx_hal.h"
#include "string.h"
#include "math.h"
#include "main.h"
#include "gprs_hal.h"
#include "gprs_app.h"
#include "gprs_comm.h"
#include "rtc.h"
#include "debug_uart.h"


int gprs_data_queue_write[CLIENT_COUNT] = {0};               
int gprs_data_queue_read[CLIENT_COUNT] = {0};

uint8_t gprs_data_buff_client[CLIENT_COUNT][GPRS_DATA_BUFF_LENGH]; 

uint8_t task_send_gprs_data_enable = 0;

uint8_t btn_event = 0;

uint8_t wake_up_who = WAKE_UP_POWER;   //通过那个方式wakeup




void btn_handle(void);




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
	
	btn_handle();
	
}




void task_send_gprs_data(void)
{
	if(task_send_gprs_data_enable == 0)
		return;
	
	if(gprs_send_data(0,"hello world",11,0) == 0)
		task_send_gprs_data_enable = 0;
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
				if(wake_up_who == WAKE_UP_POWER && sys_time.ms_count < 2000)
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




void sys_start_led_flash(void)
{
	if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0) == GPIO_PIN_SET || wake_up_who == WAKE_UP_RTC_ALARM || wake_up_who == WAKE_UP_KEY) //按下按键 或者是alarm唤醒
	{
		return;
	}
	
	if(sys_time.ms_count <500)
	{
		led_ctrl(LED_R,LED_ON);	
	}
	else if(sys_time.ms_count >500 && sys_time.ms_count <1000)
	{
		led_ctrl(LED_G,LED_ON);	
		led_ctrl(LED_R,LED_OFF);
	}
	else if(sys_time.ms_count >1000 && sys_time.ms_count <1500)
	{
		led_ctrl(LED_B,LED_ON);	
		led_ctrl(LED_G,LED_OFF);	
		led_ctrl(LED_R,LED_OFF);		
	}	
	else 
	{	
	
	}
}




void btn_handle(void)
{
	if(btn_event == BTN_LONG_EVENT)
	{
		btn_event = 0;
		enter_standby();
	}
}






