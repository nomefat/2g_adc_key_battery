#ifndef GPRS_APP_H_
#define GPRS_APP_H_

#pragma anon_unions


#define GPRS_DATA_BUFF_LENGH 4096      //服务下下发的数据区的buff


#define CLIENT_COUNT 1  //定义支持的链接数量


#define BTN_LONG_EVENT  2
#define BTN_SHORT_EVENT 1


#define WAKE_UP_KEY            1
#define WAKE_UP_RTC_ALARM      2
#define WAKE_UP_POWER          3



typedef struct _gprs_task_manage
{
    struct 
    {
        int8_t send_adc_data_enable;
            #define ENABLE_SEND                  1
            #define SEND_OK                      2
        uint8_t task_setp;
            #define TASK_NONE                       0
            #define WAIT_ADC_DATA_READY             1
            #define SEND_ADC_DATA_ING               2
            #define WAIT_PROCOTAL_SEND_OK           3           
        uint32_t make_adc_error_times;
        int32_t error_to_reboot_mod_times;
    };
        
}struct_gprs_task_manage;
















extern uint8_t wake_up_who;   //通过那个方式wakeup
extern uint8_t task_send_gprs_data_enable;

int8_t gprs_data_write_queue(uint8_t c,uint8_t data);


int8_t gprs_data_read_queue(uint8_t c,uint8_t *pdata);

void task_send_gprs_data(void);

void task_gprs_app(void);
	

int8_t get_2g_send_adc_data_stat(void);

void start_2g_send_adc_data_task(void);

void wait_2g_adc_data_send_ok(void);










#endif


