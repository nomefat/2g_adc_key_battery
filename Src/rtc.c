#include "main.h"
#include "rtc.h"
#include "gprs_app.h"
#include "debug_uart.h"
#include "gprs_comm.h"
#include "gprs_hal.h"








volatile uint8_t wake_up_who = WAKE_UP_POWER;   //通过那个方式wakeup



struct_sys_time sys_time;  //系统时间 包括秒和毫秒

const uint8_t Days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const uint16_t monDays[12] = {0,31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
 
extern RTC_HandleTypeDef hrtc;
extern uint8_t bule_switch;  //蓝牙关闭

extern void if_need_write_param(void);
	
uint8_t rtc_alarm_minuter = 10;

//秒中断回调函数
void HAL_RTCEx_RTCEventCallback(RTC_HandleTypeDef *hrtc)
{
	sys_time.ms = 0;
	HAL_RTC_GetTime(hrtc,&sys_time.sTime,RTC_FORMAT_BIN);
	HAL_RTC_GetDate(hrtc,&sys_time.sDate,RTC_FORMAT_BIN);
	sys_time.sec = make_unix_sec(sys_time.sDate,sys_time.sTime);
	
}


//通过日期时间制作unix时间戳
uint32_t make_unix_sec(RTC_DateTypeDef date, RTC_TimeTypeDef time)    //北京时间转时间戳
{
	uint32_t result;
	uint16_t Year=date.Year+2000;
	result = (Year - 1970) * 365 * 24 * 3600 + (monDays[date.Month-1] + date.Date - 1) * 24 * 3600 + (time.Hours-8) * 3600 + time.Minutes * 60 + time.Seconds;

	result += (date.Month>2 && (Year % 4 == 0) && (Year % 100 != 0 || Year % 400 == 0))*24*3600;	//闰月

	Year -= 1970;
	result += (Year/4 - Year/100 + Year/400)*24 * 3600;		//闰年
	return result;
}
 

//通过时间戳校准RTC
void adjust_datetime_from_sec(uint32_t stamp)        //时间戳转北京时间 并且校准时间
{
	RTC_DateTypeDef date;
	RTC_TimeTypeDef time;
	uint32_t days; 
	uint16_t leap_num;

	time.Seconds = stamp % 60;
	stamp /= 60;	//获取分
	time.Minutes = stamp % 60;
	stamp += 8 * 60 ;
	stamp /= 60;	//获取小时
	time.Hours = stamp % 24;
	days = stamp / 24;
	leap_num = (days + 365) / 1461;
	if( ((days + 366) % 1461) == 0) 
	{
			date.Year = (days / 366)+ 1970 -2000;
			date.Month = 12;
	date.Date = 31;  
	} else
{
	days -= leap_num; 
	date.Year = (days / 365) + 1970 -2000;
	days %= 365;
	days += 1;
	if(((date.Year%4) == 0 ) && (days==60))
	{
		date.Month = 2; 
		date.Date = 29; 
	}else
	{ 
		if(((date.Year%4) == 0 ) && (days > 60)) --days;
		for(date.Month = 0;Days[date.Month] < days;date.Month++) 
		{ 
			days -= Days[date.Month]; 
		} 
		++date.Month;
		date.Date = days;
	}
}
	HAL_RTC_SetTime(&hrtc,&time,RTC_FORMAT_BIN);	
	HAL_RTC_SetDate(&hrtc,&date,RTC_FORMAT_BIN);	//year,month,date,week:(1~7)

}




uint32_t get_ms_count(void)
{
	return sys_time.ms_count;
}

uint32_t get_sec_count(void)
{
	return sys_time.sec;
}


void set_alarm_it(void)
{
	RTC_AlarmTypeDef sAlarm; 
	
	if(rtc_alarm_minuter == 10)
	{
		sAlarm.AlarmTime.Hours = sys_time.sTime.Hours;
		sAlarm.AlarmTime.Minutes = (sys_time.sTime.Minutes+10)/10*10;
		if(sAlarm.AlarmTime.Minutes == 60)
		{
			sAlarm.AlarmTime.Minutes = 0;
			sAlarm.AlarmTime.Hours++;
			if(sAlarm.AlarmTime.Hours>23)
				sAlarm.AlarmTime.Hours = 0;
		}
	}
	else if(rtc_alarm_minuter == 30)
	{
		if(sys_time.sTime.Minutes<30)
		{	
			sAlarm.AlarmTime.Minutes = 30;				
		}
		else if(sys_time.sTime.Minutes>=30)
		{
			sAlarm.AlarmTime.Hours = sys_time.sTime.Hours+1;	
			sAlarm.AlarmTime.Minutes = 0;	
			if(sAlarm.AlarmTime.Hours>23)
				sAlarm.AlarmTime.Hours = 0;	
		}
	}
	sAlarm.AlarmTime.Seconds = 0;

	HAL_RTC_SetAlarm_IT(&hrtc,&sAlarm,RTC_FORMAT_BIN);	
	
}


//10分钟的闹钟  使能adc采样 或者 定时2G数据发送
void rtc_alarm_enable(void)
{
	
	if(wake_up_who == WAKE_UP_POWER)
	{
		wake_up_who = WAKE_UP_RTC_ALARM;
		copy_string_to_double_buff("wake up from rtc alarm\r\n");
	}
	else	
		copy_string_to_double_buff("now get rtc alarm no wake up\r\n");
	
	
	
	
}




void rtc_init(void)
{
	
	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);
	HAL_RTCEx_SetSecond_IT(&hrtc);
	HAL_RTCEx_RTCEventCallback(&hrtc);
	set_alarm_it();

}







//闹钟回调函数
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
	set_alarm_it();  //重启闹钟
	rtc_alarm_enable();  
	
}







//进入关机模式 按键会唤醒系统
void enter_stop(void)
{
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




