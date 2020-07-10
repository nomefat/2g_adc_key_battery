#ifndef GPRS_COMM_H_
#define GPRS_COMM_H_




#define CMD_CALLBACK_LIST_BEGIN const struct _cmd_list cmd_list_4g[] = {NULL,NULL,
#define CMD_CALLBACK_LIST_END NULL,NULL};
#define CMD_CALLBACK(cmd_string,callback)	cmd_string,callback,

struct _cmd_list{
	char *cmd;
	void (*func)(const char *param);
};


#define GPRS_DEBUG_PRINT

#define GPRS_STR_QUEUE_LENGH 4096      //串口接收GPRS模块的数据buff

#define GPRS_SEND_DATA_BUFF_LENGH 256

#define GPRS_AT_STR_REV_LEN 512

#define GPRS_CONNECT_FAIL_TIMEOUT   2

//#define ONE_CLIENT              //仅支持单链接


#define AT_CMD_ATI             "ATI\r\n"          //AT指令 判断GSM模块是否启动
#define AT_CMD_AT_CSQ         "AT+CSQ\r\n"      //信号强度
#define AT_CMD_AT_CREG  			"AT+CREG?\r\n"    //GSM网络是否注册    +CREG: 0,1  // <stat>=1,GSM网络已经注册上
#define AT_CMD_AT_CGREG       "AT+CGREG?\r\n"   //GPRS网络是否注册   +CGREG: 0,1    // <stat>=1,GPRS网络已经注册上
#define AT_CMD_AT_SET_BAUD    "AT+IPR=115200&W\r\n"   //配置固定波特率

#ifdef ONE_CLIENT
	#define AT_CMD_AT_QIMUX       "AT+QIMUX=0\r\n"   //配置多链接模式
	#define AT_CMD_AT_SEND         "AT+QISEND=%d\r\n"       //发送数据  连接编号 数量	
	#define AT_CMD_AT_CLOSE_CONNECT "AT+QICLOSE\r\n"    //关闭指定的tcp链接
	#define AT_CMD_AT_QISACK      "AT+QISACK\r\n"        //查询发送的数据
#else
	#define AT_CMD_AT_QIMUX       "AT+QIMUX=1\r\n"   //配置多链接模式
	#define AT_CMD_AT_SEND         "AT+QISEND=%d,%d\r\n"       //发送数据  连接编号 数量	
	#define AT_CMD_AT_QISACK      "AT+QISACK=%d\r\n"        //查询发送的数据
	#define AT_CMD_AT_CLOSE_CONNECT "AT+QICLOSE=%d\r\n"    //关闭指定的tcp链接
#endif

#define AT_CMD_AT_QISTAT      "AT+QISTATE\r\n"


#define AT_CMD_AT_CGATT         "AT+CGATT?\r\n"       //gprs 附着
#define AT_CMD_AT_SET_CGATT         "AT+CGATT=1\r\n"       //gprs 附着
#define AT_CMD_AT_SET_CGREG       "AT+CGREG=1\r\n" 
#define AT_CMD_AT_QNTP           "AT+QNTP\r\n"

#define AT_CMD_AT_CCLK           "AT+CCLK?\r\n"



#define GPRS_ERROR_START_ERROR                  99     //开机没有收到 RDY CFUN  SMS Ready 等开机自动发送的指令
#define GPRS_ERROR_NO_ATI 						100    //ATI 指令没有回应
#define GPRS_ERROR_CSQ_ERROR 				    101   //信号强度不达标
#define GPRS_ERROR_CREG_ERROR 				    102   //基站连接不上
#define GPRS_ERROR_CGREG_ERROR            103
#define GPRS_ERROR_CGATT_ERROR             104
#define GPRS_ERROR_CONNECT_ERROR     			105   //tcp连接失败
#define GPRS_ERROR_WAIT_READY_ERROR 			106   //没有等到  >
#define GPRS_ERROR_RE_CONNECT_3_NO_SENDOK 		107   //发送没有等待send ok



//#define GPRS_ERROR_NO_ATI 108
//#define GPRS_ERROR_NO_ATI 109
//#define GPRS_ERROR_NO_ATI 110
//#define GPRS_ERROR_NO_ATI 111



void callback_fun_at_ok(const char *pstr);
void callback_fun_cgreg(const char *pstr);
void callback_fun_csq(const char *pstr);
void callback_fun_ready_send(const char *pstr);
void callback_fun_creg(const char *pstr);
void callback_fun_cgatt(const char *pstr);
void callback_fun_rdy(const char *pstr);
void callback_fun_call_ready(const char *pstr);
void callback_fun_qistate(const char *pstr);
void callback_fun_qisack(const char *pstr);
void callback_fun_receive(const char *pstr);
void callback_fun_cereg(const char *pstr);
void callback_fun_0_connect_ok(const char *pstr);      //+CIPOPEN:SUCCESS,1
void callback_fun_0_connect_fail(const char *pstr);      //:SUCCESS
void callback_fun_0_closed(const char *pstr);    //:SUCCESS,1,20,4
void callback_fun_error(const char *pstr);
void callback_fun_cfun(const char *pstr);
void callback_fun_sms_ready(const char *pstr);
void callback_fun_sendok(const char *pstr);
void callback_fun_qntp(const char *pstr);
void callback_fun_cclk(const char *pstr);




typedef struct _gprs_stat{
	unsigned int start_sec_count;
	unsigned int stop_sec_count;
	unsigned int send_next_at_cmd_time_ms;	
	char start_enable;
	char error_need_to_reboot;
	unsigned int error_need_to_reboot_sec;   //超时后 就算数据没有发送完 也必须关闭模块了
	unsigned int reboot_times;
	char error_code;
	char ati_ok;
	char creg_ok;       
	char cgreg_ok;  
	char cgatt_ok;
	char qntp_ok;
	char csq;   
	char power_status;
	unsigned int rdy_count;  //
	char send_at_cmd_times;
	struct {
		char connect_ok;	
		char send_setp;
		char no_send_ok_times;           //没有收到send ok 的次数  大于3次 关闭链接 重新连接
		char re_connect_to_send_times;   //发送不成功导致的关闭连接 重连次数，大于3次重启模块
		#define SEND_STEP_NONE         0
		#define SEND_STEP_WAIT_READY   1
		#define SEND_STEP_WAIT_SEND_OK 2
		unsigned short send_data_len; 
		unsigned char send_data_buff[1500];		
	}con_client[CLIENT_COUNT];
	
	signed char rev_client;        
	unsigned short rev_len;
	int get_data_error_times;
              
}struct_gprs_stat;

extern struct_gprs_stat gprs_stat;

void stop_gprs_mod(void);

void start_gprs_mod(void);
	
void task_gprs_comm(void);

int32_t gprs_send_data(uint8_t client,void *pdata,uint16_t len,int32_t *cmd);

void gprs_get_aisack(void);















#endif

