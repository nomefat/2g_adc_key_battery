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

#define CHANGE_STOP_MAX_SEND_PACKET_COUNT 10
#define AUTO_STUDY_MAX_SEND_PACKET_COUNT  200

#define NO_ACTIVATE_MODE_MAX_TIME_MS   					(1000*60*3)      //未激活模式最多持续时间   超时即关机
#define UPDATA_MODE_MAX_TIME_MS									(1000*60*3)      //升级模式最多持续时间   
				
uint8_t power_off_flag = 0;
extern uint8_t rtc_alarm_minuter;


typedef struct _work_mode
{
    uint8_t mode;
        #define NO_ACTIVATE     0                   //未激活模式
        #define AUTO_STUDY      1                   //自动学习模式
        #define NORMAL_WORK     2                   //正常工作模式

		uint8_t dev_check_start_or_stop;           //现在检测到机器是启动还是停止的状态
			#define MECHINE_START    1
			#define MECHINE_STOP     2
		uint8_t change_stop_send_adc_data_packet_count;  //机器停止后上传的包数  大于10次后停止上传
		uint16_t auto_study_send_adc_data_packet_count;  //自动学习状态上传200包后转为正常工作状态
    struct _no_activate_mode
    {
        int8_t send_adc_data_count;                 //发送数据的次数  
        int8_t send_ok_time_ms;                     //发送成功的时刻， 小于等于0后将会关机 而且 发送成功后等待5秒没有收到应答 将会关机
        int8_t rev_if_activate_cmd;                 //收到服务器不激活的指令后置一
            #define ACTIVATED          1
            #define NO_ACTIVATED        2
        int8_t now_steps;
            #define ADC_DATA_NONE               0
            #define WAIT_ADC_DATA_SEND_OK       1
            #define ADC_DATA_SEND_OK            2
    }no_activate_mode;

    struct _auto_study_mode
    {
			int8_t now_steps;
        /* data */
    }auto_study_mode;

    struct _work_mode_
    {
			int8_t now_steps;
        /* data */
    }normal_work_mode;

}struct_work_mode;


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
		
		if(get_ms_count() < ms)
			return;
		
    if( get_ms_count() > NO_ACTIVATE_MODE_MAX_TIME_MS) //超时 
    {
				if(work_mode.no_activate_mode.rev_if_activate_cmd == ACTIVATED)  //已经激活
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
					ms = get_ms_count();	
					power_off_flag = 1;
					led_ctrl(LED_R,LED_ON);
					led_ctrl(LED_G,LED_OFF);
					led_ctrl(LED_B,LED_OFF);					
					while(get_ms_count() < (ms+5000));
					led_ctrl(LED_R,LED_OFF);
					enter_stop();  //关机
				}
    }
    else if(work_mode.no_activate_mode.rev_if_activate_cmd == NO_ACTIVATED) //接收到不激活的指令
    {
        //亮蓝灯5秒   
        ms = get_ms_count();
				power_off_flag = 1;
        led_ctrl(LED_B,LED_ON);
				led_ctrl(LED_R,LED_OFF);
				led_ctrl(LED_G,LED_OFF);			
        while(get_ms_count() < (ms+5000));
        led_ctrl(LED_B,LED_OFF);
        enter_stop();  //关机
    }
    else if(work_mode.no_activate_mode.send_adc_data_count > 0)//激活的过程
    {
        if(work_mode.no_activate_mode.now_steps == ADC_DATA_NONE) // 获取ADC数据 启动2g模块
        {
            start_2g_send_adc_data_task();
            work_mode.no_activate_mode.now_steps = WAIT_ADC_DATA_SEND_OK;
        }
        else if(work_mode.no_activate_mode.now_steps == WAIT_ADC_DATA_SEND_OK)  //等待发送成功
        {
            if(SEND_OK == get_2g_send_adc_data_stat())
            {

                work_mode.no_activate_mode.now_steps = ADC_DATA_SEND_OK;
            }
        }
        else if(work_mode.no_activate_mode.now_steps == ADC_DATA_SEND_OK)    //sendok 
        {
            work_mode.no_activate_mode.send_adc_data_count--;
            work_mode.no_activate_mode.now_steps = ADC_DATA_NONE;
            if( work_mode.no_activate_mode.send_adc_data_count<=0) //发送完成
            {
                work_mode.no_activate_mode.send_adc_data_count = -1;
                if(work_mode.no_activate_mode.rev_if_activate_cmd == ACTIVATED)  //已经激活
                {
									//不在这里处理  收到服务器的服务完成ack包后在处理
//                    work_mode.mode = AUTO_STUDY;
//                    rtc_alarm_minuter = 30;
//                    //亮绿灯5秒   
//                    ms = get_ms_count();
//										power_off_flag = 1;
//                    led_ctrl(LED_G,LED_ON);
//                    while(get_ms_count() < (ms+5000));
//                    led_ctrl(LED_G,LED_OFF);
//                    enter_standby();  //睡眠                
                }
            }
        }     
    }
    
    
}



/*
* 功能:自动学习模式轮训函数




*/
void auto_study_mode_in(void)
{
    rtc_alarm_minuter = 30;    
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


void work(void)
{
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
            led_flash(LED_B);
        return;
    }   
    if(work_mode.mode == NO_ACTIVATE)
    {
        if(sys_time.ms_count%100 == 0)
            led_flash(LED_R);
    }
    else if(work_mode.mode == AUTO_STUDY)
    {
        if(sys_time.ms_count%500 == 0)
            led_flash(LED_R);
    }
    else if(work_mode.mode == NORMAL_WORK)
    {
        if(sys_time.ms_count%1500 == 0)
            led_flash(LED_R);
    }
}




void dead_in_key_down(void)
{
	uint8_t key;
	uint32_t ms;
	
	while(get_ms_count() < 100);
	
	led_ctrl(LED_R,LED_ON);
	while(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0) == GPIO_PIN_SET); //按键按下  死在这里
	led_ctrl(LED_R,LED_OFF);
	
	ms = get_ms_count();
	if(ms>2000) //长按键
	{
		key = BTN_LONG_EVENT;
	}
	else 
	{
		key = BTN_SHORT_EVENT;
	}
	
	if(work_mode.mode == NO_ACTIVATE)
	{	
		if(key == BTN_SHORT_EVENT)
			enter_standby();  //睡眠 
		else
		{
      work_mode.no_activate_mode.send_adc_data_count = 3;		
		}
	}
	else if(work_mode.mode == AUTO_STUDY)
	{
			if((sys_time.sTime.Minutes == 0 || sys_time.sTime.Minutes == 30)&& sys_time.sTime.Seconds < 5)
			{
				start_ss_check = 1; //检测启停
			}
			else
				enter_standby();  //睡眠
	}
	else if(work_mode.mode == NORMAL_WORK)
	{
			if((sys_time.sTime.Minutes%10 == 0)&& sys_time.sTime.Seconds < 5)
			{
				start_ss_check = 1; //检测启停				
			}
			else
				enter_standby();  //睡眠
	}
		
}











