#ifndef _WORK_MODE_
#define _WORK_MODE_




#pragma anon_unions

#define PARAM_ADDR (0X08000000+16*1024+200*1024)   	//固件存放地址

#define CHANGE_STOP_MAX_SEND_PACKET_COUNT 10
#define AUTO_STUDY_MAX_SEND_PACKET_COUNT  200

#define NO_ACTIVATE_MODE_MAX_TIME_MS   					(1000*60*3)      //未激活模式最多持续时间   超时即关机
#define UPDATA_MODE_MAX_TIME_MS									(1000*60*3)      //升级模式最多持续时间   
				

typedef struct _work_mode
{
    uint16_t len;
    #define LEN_PARAM       14
    uint16_t crc;
    struct
    {
        uint8_t mode:4;
        #define NO_ACTIVATE     0                   //未激活模式
        #define AUTO_STUDY      1                   //自动学习模式
        #define NORMAL_WORK     2                   //正常工作模式
			uint8_t dev_stat:4;       					//表示设备是开机还是关机状态
			#define DEV_RUNING 0                 //开机
			#define DEV_STOPING 1              //关机
    };
    uint8_t dev_check_start_or_stop;           //现在检测到机器是启动还是停止的状态
        #define MECHINE_START    1
        #define MECHINE_STOP     2
    uint8_t change_stop_send_adc_data_packet_count;  //机器停止后上传的包数  大于10次后停止上传
    uint16_t auto_study_send_adc_data_packet_count;  //自动学习状态上传200包后转为正常工作状态
	uint8_t nothing;
	uint32_t adc_data_send_ok_sec;  //记录采样数据发送成功的时刻
	
	uint32_t dev_send_count;

    struct _no_activate_mode
    {
        int8_t send_adc_data_count;                 //发送数据的次数  
        int8_t send_ok_time_ms;                     //发送成功的时刻， 小于等于0后将会关机 而且 发送成功后等待5秒没有收到应答 将会关机
        int8_t rev_if_activate_cmd;                 //收到服务器不激活的指令后置一
			#define NEED_ACTIVATE         0  //需要激活
			#define ACTIVATE_ING          1  //已经发送激活包 等待激活			
			#define ACTIVATE_OK           2  //已经激活  还需要发送3包数据
            #define NO_ACTIVATED          3 //收到服务器不激活的指令
        int8_t now_steps;
            #define ADC_DATA_NONE               0
            #define WAIT_ADC_DATA_SEND_OK       1
            #define ADC_DATA_SEND_OK            2
		int8_t dev_self_test;
			#define NONE_SELF_TEST        0  //无需操作
			#define NEED_SELF_TEST        1  //需要自测成功
			#define SELF_TEST_ING         2  //已发送自测数据包 等待服务器 ack
			#define SELE_TEST_OK          3  //已自测成功    		 	
    }no_activate_mode;

    struct _auto_study_mode
    {
		int8_t now_steps;
		int8_t ask_server_if_need_cfg;
		int8_t dev_stop_send_to_server;
        /* data */
    }auto_study_mode;

    struct _work_mode_
    {
		int8_t now_steps;
		int8_t ask_server_if_need_cfg;	
		int8_t dev_stop_send_to_server;			
        /* data */
    }normal_work_mode;

}struct_work_mode;





extern struct_work_mode work_mode;






void get_server_self_test_ok(void);

void get_server_activate_faild(void);

void get_server_activate_ok(void *pdata);

void get_server_activate_ok_adc_data_ack(void);



















#endif
