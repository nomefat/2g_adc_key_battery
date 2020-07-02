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



extern uint8_t rtc_alarm_minuter;


typedef struct _work_mode
{
    uint8_t mode;
        #define NO_ACTIVATE     0                   //未激活模式
        #define AUTO_STUDY      1                   //自动学习模式
        #define NORMAL_WORK     2                   //正常工作模式


    struct _no_activate_mode
    {
        #define MODE_MAX_TIME_MS   (1000*60*3)      //模式最多持续时间   超时即关机
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


/*
* 功能:未激活模式轮训函数




*/
void no_activate_mode_in(void)
{
    uint32_t ms;

    if(work_mode.no_activate_mode.send_adc_data_count == 0)
    {
        work_mode.no_activate_mode.send_adc_data_count = 3;
    }

    if( get_ms_count() > MODE_MAX_TIME_MS) //超时 
    {
        //亮红灯5秒   
        ms = get_ms_count();
        led_ctrl(LED_R,LED_ON);
        while(get_ms_count() < (ms+5000));
        led_ctrl(LED_R,LED_OFF);
        enter_stop();  //关机
    }
    else if(work_mode.no_activate_mode.rev_if_activate_cmd == NO_ACTIVATED) //接收到不激活的指令
    {
        //亮蓝灯5秒   
        ms = get_ms_count();
        led_ctrl(LED_B,LED_ON);
        while(get_ms_count() < (ms+5000));
        led_ctrl(LED_B,LED_OFF);
        enter_stop();  //关机
    }
    else //激活的过程
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
                    work_mode.mode = AUTO_STUDY;
                    rtc_alarm_minuter = 30;
                    //亮绿灯5秒   
                    ms = get_ms_count();
                    led_ctrl(LED_G,LED_ON);
                    while(get_ms_count() < (ms+5000));
                    led_ctrl(LED_G,LED_OFF);
                    enter_standby();  //睡眠                
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



void work(void)
{
    if(work_mode.mode == NO_ACTIVATE)
    {
        no_activate_mode_in();
    }
    else if(work_mode.mode == AUTO_STUDY)
    {
        no_activate_mode_in();
    }
    else if(work_mode.mode == NORMAL_WORK)
    {
        no_activate_mode_in();
    }
}





void sys_status_led_flash(void)
{	
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



















