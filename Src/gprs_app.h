#ifndef GPRS_APP_H_
#define GPRS_APP_H_




#define GPRS_DATA_BUFF_LENGH 4096      //服务下下发的数据区的buff


#define CLIENT_COUNT 1  //定义支持的链接数量


#define BTN_LONG_EVENT  2
#define BTN_SHORT_EVENT 1


#define WAKE_UP_KEY            1
#define WAKE_UP_RTC_ALARM      2
#define WAKE_UP_POWER          3


extern uint8_t wake_up_who;   //通过那个方式wakeup
extern uint8_t task_send_gprs_data_enable;

int8_t gprs_data_write_queue(uint8_t c,uint8_t data);


int8_t gprs_data_read_queue(uint8_t c,uint8_t *pdata);

void task_send_gprs_data(void);

void task_gprs_app(void);
	
#endif


