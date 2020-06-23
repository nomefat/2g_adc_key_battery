#ifndef _RTC_H_
#define _RTC_H_




typedef struct _sys_time
{
	uint32_t sec;  
	volatile uint16_t ms;    
	volatile uint32_t ms_count;
	RTC_DateTypeDef sDate;
	RTC_TimeTypeDef sTime;	
}struct_sys_time;

extern struct_sys_time sys_time;  

uint32_t make_unix_sec(RTC_DateTypeDef date, RTC_TimeTypeDef time);   

void adjust_datetime_from_sec(uint32_t stamp) ;     
	
uint32_t get_ms_count(void);

uint32_t get_sec_count(void);

void rtc_init(void);
	
void enter_standby(void);
	
int32_t make_protocal_data(void);









#endif


