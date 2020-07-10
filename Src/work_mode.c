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
#include "work_mode.h"
#include "adc.h"


#define debug(str) copy_string_to_double_buff(str)


/*
开关机和模式切换说明:

注释：按键分为长按6秒以上 短按200ms以上 再短过滤为抖动 不做处理

休眠模式(灯灭)（未知是开机还是关机）：
	按下按键：如果是开机状态 显示绿灯：松开按键，依次切换为蓝牙模式，蓝灯闪烁 或者其他模式
									继续按键持续6秒以上 变换为红灯 松开按键灯灭 关机 (2G发送一包关机状态)

			如果是关机状态 无灯亮：松开按键，，继续保持关机
								 继续按键持续6秒以上 变换为绿灯 松开按键后按照工作模式闪灯提示

开机模式(有灯)
	长按键超过6秒直到红灯常亮，松开 关机 (2G发送一包关机状态)
	短按键，依次切换为蓝牙模式，蓝灯闪烁 或者其他模式

模式灯指示:
	未激活模式  	： 未自测成功 红灯100ms闪烁  自测成功 绿灯100ms闪烁 激活成功跳转到自动学习模式
	自动学习模式    ： 绿灯500ms闪烁
	智能检测模式    :  绿灯1500ms闪烁

	蓝牙模式        ：蓝灯1000ms闪烁
*/

uint8_t power_off_flag = 0;
extern uint8_t rtc_alarm_minuter;
uint8_t btn_event = 0;

int8_t start_key_sanf;	

struct_work_mode work_mode;

uint8_t bule_switch = 0;  //蓝牙关闭

uint8_t updata_mode = 0;  //进入升级模式
uint32_t updata_mode_start_ms;  //升级模式开始的时间

uint8_t start_ss_check = 0;




/*
* 功能:未激活模式轮训函数




*/
void no_activate_mode_in(void)
{
    static uint32_t ms = 0;
	static int8_t sleep_flag = 0;

		if(get_ms_count() < ms)
			return;
		
    if( get_ms_count() > NO_ACTIVATE_MODE_MAX_TIME_MS || sleep_flag == 1) //超时 
	{
		if(work_mode.no_activate_mode.rev_if_activate_cmd == ACTIVATE_OK)  //已经激活
		{
			work_mode.mode = AUTO_STUDY;
			rtc_alarm_minuter = 30;
			//亮绿灯5秒   
			ms = get_ms_count();
			power_off_flag = 1;
			led_ctrl(LED_G,LED_ON);	
			led_ctrl(LED_R,LED_OFF);
			led_ctrl(LED_B,LED_OFF);					
			while(get_ms_count() < (ms+5000));
			led_ctrl(LED_G,LED_OFF);
			enter_standby();  //睡眠                
		}
		else
		{
			//亮红灯5秒   
			work_mode.dev_stat = DEV_STOPING; //关机
			ms = get_ms_count();	
			power_off_flag = 1;
			led_ctrl(LED_R,LED_ON);
			led_ctrl(LED_G,LED_OFF);
			led_ctrl(LED_B,LED_OFF);					
			while(get_ms_count() < (ms+5000));
			led_ctrl(LED_R,LED_OFF);
			enter_standby();  //睡眠   
		}
    }
    else if(work_mode.no_activate_mode.rev_if_activate_cmd == NO_ACTIVATED) //接收到不激活的指令
    {
        //亮蓝灯5秒   
		work_mode.dev_stat = DEV_STOPING; //关机
        ms = get_ms_count();
		power_off_flag = 1;
        led_ctrl(LED_B,LED_ON);
		led_ctrl(LED_R,LED_OFF);
		led_ctrl(LED_G,LED_OFF);			
        while(get_ms_count() < (ms+5000));
        led_ctrl(LED_B,LED_OFF);
        enter_standby();  //关机
    }
    else if(work_mode.no_activate_mode.dev_self_test == NEED_SELF_TEST) //需要自测
    {
        if(work_mode.no_activate_mode.now_steps == ADC_DATA_NONE) // 获取ADC数据 启动2g模块
        {
            start_2g_send_adc_data_task(DEV_2G_SEND_SELF_TEST_ADC_DATA,ADC_DATA_TYPE_SELF_TEST);
            work_mode.no_activate_mode.now_steps = WAIT_ADC_DATA_SEND_OK;
        }
        else if(work_mode.no_activate_mode.now_steps == WAIT_ADC_DATA_SEND_OK)  //等待发送成功
        {
            if(SEND_OK == get_2g_send_adc_data_stat())
            {
                work_mode.no_activate_mode.dev_self_test = SELF_TEST_ING;
				ms = get_ms_count();
            }
        }
	}
    else if(work_mode.no_activate_mode.dev_self_test == SELF_TEST_ING) //等待服务器自测响应
    {
		if(get_ms_count() > (ms+10000)) //10秒 没有响应进行重发
		{
			work_mode.no_activate_mode.now_steps = ADC_DATA_NONE;
			work_mode.no_activate_mode.dev_self_test = NEED_SELF_TEST;
		}
	}
    else if(work_mode.no_activate_mode.dev_self_test == SELE_TEST_OK) //自测OK 进行激活
    {
		if(work_mode.no_activate_mode.rev_if_activate_cmd == NEED_ACTIVATE) //需要激活
		{
			if(work_mode.no_activate_mode.now_steps == ADC_DATA_NONE) // 发送激活请求包
			{
				send_to_server_activate_data();
				work_mode.no_activate_mode.now_steps = WAIT_ADC_DATA_SEND_OK;
			}
			else if(work_mode.no_activate_mode.now_steps == WAIT_ADC_DATA_SEND_OK)  //等待发送成功
			{
				if(SEND_OK == get_2g_send_activate_data_stat())
				{
					work_mode.no_activate_mode.rev_if_activate_cmd = ACTIVATE_ING;
					ms = get_ms_count();
				}
			}
		}
		else if(work_mode.no_activate_mode.rev_if_activate_cmd == ACTIVATE_ING) //等待激活响应
		{
			if(get_ms_count() > (ms+10000)) //10秒 没有响应进行重发
			{
				work_mode.no_activate_mode.now_steps = ADC_DATA_NONE;
				work_mode.no_activate_mode.rev_if_activate_cmd = NEED_ACTIVATE;
			}
		} 
		else if(work_mode.no_activate_mode.rev_if_activate_cmd == ACTIVATE_OK) //已经激活
		{
			if(work_mode.no_activate_mode.send_adc_data_count > 0)//激活后发数据过程
			{
				if(work_mode.no_activate_mode.now_steps == ADC_DATA_NONE) // 获取ADC数据 启动2g模块
				{
					start_2g_send_adc_data_task(DEV_2G_SEND_ADC_DATA,ADC_DATA_TYPE_SELF_TEST);
					work_mode.no_activate_mode.now_steps = WAIT_ADC_DATA_SEND_OK;
				}
				else if(work_mode.no_activate_mode.now_steps == WAIT_ADC_DATA_SEND_OK)  //等待发送成功
				{
					if(SEND_OK == get_2g_send_adc_data_stat())
					{
						ms = get_ms_count();
						work_mode.no_activate_mode.now_steps = ADC_DATA_SEND_OK;
					}
				}
				else if(work_mode.no_activate_mode.now_steps == ADC_DATA_SEND_OK)    //sendok 
				{
					if(get_ms_count() > (ms+10000)) //10秒 没有响应进行重发
					{
						work_mode.no_activate_mode.now_steps = ADC_DATA_NONE;
					}
				}     
			}
			else
			{
				//sleep_flag = 1;
			}
			
		} 		
	}





    
    
}

//
void get_server_self_test_ok(void)
{
	work_mode.no_activate_mode.dev_self_test = SELE_TEST_OK;
	work_mode.no_activate_mode.now_steps = ADC_DATA_NONE;
	debug("get_server:self test ok\r\n");
}

//激活OK  切状态  使能发三包数据
void get_server_activate_ok(void *pdata)
{
	struct_activate_packet_ack *p_activate_packet_ack = (struct_activate_packet_ack *)pdata;

	work_mode.no_activate_mode.rev_if_activate_cmd = ACTIVATE_OK;
	work_mode.no_activate_mode.now_steps = ADC_DATA_NONE;
	work_mode.no_activate_mode.send_adc_data_count = 3;
	adjust_datetime_from_sec(to_small_endian_uint32(p_activate_packet_ack->server_time_sec));

	sprintf(debug_send_buff,"get_server:activate ok unix_time=%d band_type=%d auto_study_count=%d timer_time=%d\r\n",
		to_small_endian_uint32(p_activate_packet_ack->server_time_sec),to_small_endian_uint16(p_activate_packet_ack->band_dev_type),
		to_small_endian_uint16(p_activate_packet_ack->auto_study_send_packet_count),to_small_endian_uint16(p_activate_packet_ack->timer_send_adc_data));
	debug(debug_send_buff);
	print_date_time();
}

//服务器拒绝激活  切状态  
void get_server_activate_faild(void)
{
	work_mode.no_activate_mode.rev_if_activate_cmd = NO_ACTIVATED;
	work_mode.no_activate_mode.now_steps = ADC_DATA_NONE;
	work_mode.no_activate_mode.send_adc_data_count = 0;
	debug("get_server:activate faild\r\n");
}

//激活OK  服务器收到3包adc采集数据中的一包 所给的ack
void get_server_activate_ok_adc_data_ack(void)
{
	work_mode.no_activate_mode.now_steps = ADC_DATA_NONE;
	work_mode.no_activate_mode.send_adc_data_count--;
	debug("get_server:activate adc data ack\r\n");
}








/*
* 功能:自动学习模式轮训函数




*/
void auto_study_mode_in(void)
{
	static uint32_t ms = 0;

    rtc_alarm_minuter = 30;    
	ms = get_ms_count();
	if(ms >30000)
	{
		enter_standby();  //睡眠 		
	}
}


/*
* 功能:正常工作模式轮训函数




*/
void work_mode_in(void)
{
    rtc_alarm_minuter = 10;    
}

/*
* 功能:升级模式轮训函数




*/
void updata_mode_in(void)
{
	
}

/*
* 功能:蓝牙模式轮训函数




*/
void blue_mode_in(void)
{
	
}

void stop_mode_in(void)
{
	enter_standby();  //睡眠 
}

void work(void)
{
		if(work_mode.dev_stat == DEV_STOPING)
		{
			stop_mode_in();
			return;
		}
		if(bule_switch)  //蓝牙开启
		{
			blue_mode_in();
			return;  //蓝牙模式无法升级
		}
		if(updata_mode)  //进入升级模式,不再进入别的模式  避免别的模式超时进入睡眠
		{	
			updata_mode_in();
		}
    else if(work_mode.mode == NO_ACTIVATE)
    {
        no_activate_mode_in();
    }
    else if(work_mode.mode == AUTO_STUDY)
    {
        auto_study_mode_in();
    }
    else if(work_mode.mode == NORMAL_WORK)
    {
        work_mode_in();
    }
}





void sys_status_led_flash(void)
{	
	if(power_off_flag)
		return;
		
    if(bule_switch)
    {
         if(sys_time.ms_count%1000 == 0)
				 {
						led_ctrl(LED_G,LED_OFF);
						led_ctrl(LED_R,LED_OFF);
            led_flash(LED_B);
				 }
        return;
    }   
    if(work_mode.mode == NO_ACTIVATE && work_mode.no_activate_mode.rev_if_activate_cmd != ACTIVATE_OK)
    {
        if(sys_time.ms_count%100 == 0)
				{	
						if(work_mode.no_activate_mode.dev_self_test != SELE_TEST_OK)
						{
							led_ctrl(LED_G,LED_OFF);
							led_flash(LED_R);
						}
						else
						{
							led_ctrl(LED_R,LED_OFF);
							led_flash(LED_G);
						}
				}
    }
    else if(work_mode.mode == AUTO_STUDY ||work_mode.no_activate_mode.rev_if_activate_cmd == ACTIVATE_OK)
    {
        if(sys_time.ms_count%500 == 0)
            led_flash(LED_G);
    }
    else if(work_mode.mode == NORMAL_WORK)
    {
        if(sys_time.ms_count%1500 == 0)
            led_flash(LED_G);
    }
}




void dead_in_key_down(void)
{
	uint8_t key;
	uint32_t ms;
	
	power_off_flag = 1;
	while(get_ms_count() < 100);
	
	while(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0) == GPIO_PIN_SET) //按键按下  死在这里
	{
		if(work_mode.dev_stat == DEV_RUNING) //开机状态立马亮绿灯
		{
			if(get_ms_count() > 6000)
			{
				led_ctrl(LED_R,LED_ON);
				led_ctrl(LED_G,LED_OFF);
			}
			else
			{
				led_ctrl(LED_G,LED_ON);
			}
		}
		else              //关机状态 超过6秒后亮绿灯 表示开机了
		{
			if(get_ms_count() > 6000)
			{
				led_ctrl(LED_G,LED_ON);
			}
		}
	}
	led_ctrl(LED_G,LED_OFF);
	
	
	ms = get_ms_count();
	if(ms>6000) //长按键
	{
		key = BTN_LONG_EVENT;
		if(work_mode.dev_stat == DEV_STOPING) //关机->开机
		{
			power_off_flag = 0;
			work_mode.dev_stat = DEV_RUNING;   
			work_mode.normal_work_mode.ask_server_if_need_cfg = 1;		 //开机问询是否配置
			work_mode.auto_study_mode.ask_server_if_need_cfg = 1;			
			if(work_mode.mode == NO_ACTIVATE)
			{	
				work_mode.no_activate_mode.dev_self_test = NEED_SELF_TEST;		
			}					
		}	
		else                                  //开机->关机
		{
			work_mode.dev_stat = DEV_STOPING;
			work_mode.auto_study_mode.dev_stop_send_to_server = 1;  //关机通知服务器
			work_mode.normal_work_mode.dev_stop_send_to_server = 1;	
			power_off_flag = 1;
		}
			
	}
	else if(ms > 100)
	{
		if(work_mode.dev_stat == DEV_RUNING)
		{
			power_off_flag = 0;
			bule_switch = 1;
		}
		else
		{
			enter_standby();  //睡眠 
		}
		
	}
	else //按键太短 可能不是按键唤醒的
	{
		if(work_mode.mode == AUTO_STUDY)
		{
			if((sys_time.sTime.Minutes == 0 || sys_time.sTime.Minutes == 30)&& sys_time.sTime.Seconds < 5)
			{
				power_off_flag = 0;
				start_ss_check = 1; //检测启停
			}
			else
				enter_standby();  //睡眠
		}
		else if(work_mode.mode == NORMAL_WORK)
		{
			if((sys_time.sTime.Minutes%10 == 0)&& sys_time.sTime.Seconds < 5)
			{
				power_off_flag = 0;
				start_ss_check = 1; //检测启停				
			}
			else
				enter_standby();  //睡眠
		}
		else
			enter_standby();  //睡眠
	}
	start_key_sanf = 1;		
}

//擦除参数存储区
HAL_StatusTypeDef erase_param_store(void)
{
	FLASH_EraseInitTypeDef EraseInit;
	HAL_StatusTypeDef HAL_Status;
	uint32_t PageError;
	
	EraseInit.PageAddress = PARAM_ADDR;
	EraseInit.Banks = FLASH_BANK_1;
	EraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInit.NbPages = 1;
	
	HAL_FLASH_Unlock();
	HAL_Status = HAL_FLASHEx_Erase(&EraseInit, &PageError);	
	HAL_FLASH_Lock();
	
	return HAL_Status;
	
}


int32_t write_param(void)
{
	uint16_t i,half_word_len;
	HAL_StatusTypeDef HAL_Status;
	uint16_t *p_data = (uint16_t *)&work_mode;
	uint16_t *p_readback_data = (uint16_t *)PARAM_ADDR;
	
	work_mode.len = LEN_PARAM;
	work_mode.crc = MB_CRC16(((uint8_t *)(&(work_mode.crc)))+2,LEN_PARAM);

	erase_param_store();
	
	HAL_FLASH_Unlock();
	half_word_len = (work_mode.len+4)/2;
	for(i = 0;i<half_word_len;i++)
	{
		HAL_Status =  HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, PARAM_ADDR+i*2, p_data[i]);	
		if(p_readback_data[i] !=  p_data[i])
		{
			HAL_Status =  HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, PARAM_ADDR+i*2, p_data[i]);	
			if(p_readback_data[i] !=  p_data[i])
			{
				HAL_Status =  HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, PARAM_ADDR+i*2, p_data[i]);	
				HAL_FLASH_Lock();	
				return -1;
			}
		}
	}
	HAL_FLASH_Lock();	
	return 0;
}

void if_need_write_param(void)
{
	uint16_t i,half_word_len;
	uint8_t *p_data = (uint8_t *)&work_mode;
	uint8_t *p_readback_data = (uint8_t *)PARAM_ADDR;
	half_word_len = (work_mode.len+4)/2;
	
	for(i = 0;i<half_word_len;i++)
	{
		if(p_readback_data[i] !=  p_data[i])	
		{
			if(write_param() == 0)
			{
				return;
			}
		}
	}
}


void read_param(void)
{
	struct_work_mode *p_param_flash = (struct_work_mode *)PARAM_ADDR;
	
	if(p_param_flash->len > 2048)
	{
		goto WRITE_PARAM;
	}
	if(MB_CRC16(((uint8_t *)(&(p_param_flash->crc)))+2,LEN_PARAM) != p_param_flash->crc)
	{
		goto WRITE_PARAM;
	}		
	
	memcpy(&work_mode,p_param_flash,LEN_PARAM);
	return;
	
WRITE_PARAM:
	work_mode.mode = NO_ACTIVATE;
	work_mode.dev_stat = DEV_STOPING;
	work_mode.auto_study_send_adc_data_packet_count = 0;
	work_mode.change_stop_send_adc_data_packet_count = 0;
	work_mode.dev_check_start_or_stop = MECHINE_STOP;

	write_param();
}



//进入休眠模式,RTC闹钟和按键会唤醒系统
void enter_standby(void)
{

	if(work_mode.mode == NO_ACTIVATE && work_mode.no_activate_mode.rev_if_activate_cmd == ACTIVATE_OK)
	{
		work_mode.mode = AUTO_STUDY;		
	}
	
	if_need_write_param();
	if_need_write_param();
	if_need_write_param();	
	stop_gprs_mod();
	led_ctrl(LED_R,LED_OFF);
	led_ctrl(LED_G,LED_OFF);
	led_ctrl(LED_B,LED_OFF);	
	HAL_GPIO_WritePin(SENSOR_PWR_CTRL_GPIO_Port,SENSOR_PWR_CTRL_Pin,GPIO_PIN_RESET);	
	
	HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
	__disable_irq();
	HAL_PWR_EnterSTANDBYMode();
		
}




extern int8_t start_key_sanf;	
extern uint8_t power_off_flag;
void button_scan(void)
{
	 static uint32_t down_ms = 0;
	 static uint32_t up_ms = 0;	
	if(start_key_sanf == 0)
		return;
		
	if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0) == GPIO_PIN_RESET) //松开
	{
		up_ms++;
		if(up_ms > 100) //判断为松开按键
		{
			if(down_ms > 6000) //长按键
			{
				btn_event = BTN_LONG_EVENT;
				down_ms = 0;		
			}
			else if(down_ms > 100) //短按键
			{
				btn_event = BTN_SHORT_EVENT;	
				down_ms = 0;			
			}
			up_ms = 0;	
		}
	}
	else  //按下
	{
		down_ms++;
		
		if(down_ms > 6000) //长按键
		{				
			power_off_flag = 1;
			led_ctrl(LED_G,LED_OFF);	
			led_ctrl(LED_R,LED_ON);
			led_ctrl(LED_B,LED_OFF);			
		}
		else if(down_ms > 100 ) //短按键
		{					
			up_ms = 0;			
		}
	}
	
}



void btn_handle(void)
{

	
	if(btn_event == BTN_LONG_EVENT) //长按键
	{
		btn_event = 0;
		power_off_flag = 1;
		led_ctrl(LED_G,LED_OFF);	
		led_ctrl(LED_R,LED_OFF);
		led_ctrl(LED_B,LED_OFF);		
		work_mode.dev_stat = DEV_STOPING;
		work_mode.auto_study_mode.dev_stop_send_to_server = 1;  //关机通知服务器
		work_mode.normal_work_mode.dev_stop_send_to_server = 1;			
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




