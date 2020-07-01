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






typedef struct _work_mode
{
    uint8_t mode;
        #define NO_ACTIVATE     0    //未激活模式
        #define AUTO_STUDY      1    //自动学习模式
        #define NORMAL_WORK            2    //正常工作模式


    struct _no_activate_mode
    {
        #define MODE_MAX_TIME_MS   (1000*60*3)      //模式最多持续时间   超时即关机
        int8_t send_adc_data_count;       //发送数据的次数  
        int8_t send_ok_time_ms;           //发送成功的时刻， 小于等于0后将会关机 而且 发送成功后等待5秒没有收到应答 将会关机
        int8_t rev_no_activate_cmd;      //收到服务器不激活的指令后置一
        int8_t now_steps;
            #define ADC_DATA_NONE

    }no_activate_mode;

    struct _auto_study_mode
    {
        /* data */
    }auto_study_mode;

    struct _work_mode
    {
        /* data */
    }normal_work_mode;

}struct_work_mode;


struct_work_mode work_mode;








void no_activate_mode_in(void)
{
    uint32_t ms;

    if(work_mode.no_activate_mode.send_adc_data_count <= 0 || get_ms_count() > MODE_MAX_TIME_MS) //重发次数用完 或者超时
    {
        if(work_mode.no_activate_mode.send_ok_time_ms != 0)  //发送成功过
        {
            if(get_ms_count() > (work_mode.no_activate_mode.send_ok_time_ms + 5000)) //等待5秒 未收到激活指令
            {
                //亮红灯5秒   
                ms = get_ms_count();
                led_ctrl(LED_R,LED_ON);
                while(get_ms_count() < (ms+5000));
                led_ctrl(LED_R,LED_OFF);
                enter_stop();  //关机
            }
        }
        else
        {
            //亮红灯5秒   
            ms = get_ms_count();
            led_ctrl(LED_R,LED_ON);
            while(get_ms_count() < (ms+5000));
            led_ctrl(LED_R,LED_OFF);
            enter_stop();  //关机
        }  
    }
    else if(work_mode.no_activate_mode.rev_no_activate_cmd == 1) //接收到不激活的指令
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
        if(work_mode.no_activate_mode.now_steps == ADC_DATA_NONE) //
        {

        }
    }
    
    
}


void auto_study_mode_in(void)
{
    
}


void work_mode_in(void)
{
    
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





















