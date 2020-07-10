#include "stm32f1xx_hal.h"
#include "string.h"
#include "stdio.h"
#include "math.h"
#include "gprs_hal.h"
#include "gprs_app.h"
#include "gprs_comm.h"
#include "rtc.h"
#include "debug_uart.h"
#include "communication_protocol_handle.h"




uint32_t gprs_server_ip = (101<<24)|(200<<16)|(39<<8)|204;
uint16_t gprs_server_port = 20008;

//uint32_t gprs_server_ip = (219<<24)|(239<<16)|(83<<8)|74;
//uint16_t gprs_server_port = 40010;


  
int gprs_str_queue_write = 0;               
int gprs_str_queue_read = 0;								


int gprs_receive_packet_flag = 0;


int gprs_cmd_param_num = 0;
unsigned char gprs_cmd_param[10][50];  
uint8_t gprs_sec_flag = 0;         

uint8_t gprs_str_queue[GPRS_STR_QUEUE_LENGH];  
char gprs_debug_buff[256];
char at_cmd_conn[50];
char at_cmd_str_buf[GPRS_AT_STR_REV_LEN];   //容纳一行读的AT字符串
  
extern char debug_send_buff[Q_LEN];
	
extern struct_adc_data_param adc_data_param;  //adc数据块的一些参数 
 
int gprs_print_rx_tx_data_enable = 0;   

struct_gprs_stat gprs_stat;

CMD_CALLBACK_LIST_BEGIN

CMD_CALLBACK("OK",callback_fun_at_ok)
CMD_CALLBACK("+CGREG",callback_fun_cgreg)
CMD_CALLBACK("+CSQ",callback_fun_csq)
CMD_CALLBACK(">",callback_fun_ready_send)
CMD_CALLBACK("+CREG",callback_fun_creg)
CMD_CALLBACK("+CGATT",callback_fun_cgatt)
CMD_CALLBACK("RDY",callback_fun_rdy)
CMD_CALLBACK("+CFUN",callback_fun_cfun)
CMD_CALLBACK("SMS Ready",callback_fun_sms_ready)
CMD_CALLBACK("Call Ready",callback_fun_call_ready)
#ifdef ONE_CLIENT
CMD_CALLBACK("CONNECT OK",callback_fun_0_connect_ok)      //+CIPOPEN:SUCCESS,1
CMD_CALLBACK("CONNECT FAIL" ,callback_fun_0_connect_fail)      //:SUCCESS
CMD_CALLBACK("CLOSED",callback_fun_0_closed)    //:SUCCESS,1,20,4
#else
CMD_CALLBACK("0, CONNECT OK",callback_fun_0_connect_ok)      //+CIPOPEN:SUCCESS,1
CMD_CALLBACK("0, CONNECT FAIL" ,callback_fun_0_connect_fail)      //:SUCCESS
CMD_CALLBACK("0, CLOSED",callback_fun_0_closed)    //:SUCCESS,1,20,4
#endif
CMD_CALLBACK("+RECEIVE",callback_fun_receive)  //:0
CMD_CALLBACK("ERROR",callback_fun_error)
CMD_CALLBACK("+QISTATE" , callback_fun_qistate) //:0CMD_CALLBACK(
CMD_CALLBACK("+QISACK",callback_fun_qisack)          //:SUCCESS,0,0,19,callback_fun_help)
CMD_CALLBACK("SEND OK",callback_fun_sendok)          //:SUCCESS,0,0,19,callback_fun_help)
CMD_CALLBACK("+QNTP",callback_fun_qntp)  
CMD_CALLBACK("+CCLK",callback_fun_cclk)
CMD_CALLBACK_LIST_END



/*
 * 
 * 
 * 
*/
int8_t gprs_str_write_queue(uint8_t data)
{
	if((gprs_str_queue_write+1)%GPRS_STR_QUEUE_LENGH==gprs_str_queue_read)
		return -1;
	gprs_str_queue[gprs_str_queue_write] = data;
	gprs_str_queue_write = (gprs_str_queue_write+1)%GPRS_STR_QUEUE_LENGH;
	return 0;
}

/*
 * 
 * 
 * 
*/
int8_t gprs_str_read_queue(uint8_t *pdata)
{
	if(gprs_str_queue_write==gprs_str_queue_read)
		return -1;		
	*pdata = gprs_str_queue[gprs_str_queue_read];
	gprs_str_queue_read = (gprs_str_queue_read+1)%GPRS_STR_QUEUE_LENGH;
	return 0;
}

/*

*/
void gprs_str_copy_to_queue(unsigned short len,char* p_data)
{
	int i = 0;

	for(i=0;i<len;i++)
	{
		gprs_str_write_queue(*p_data++);	
	}

}





char at_cmd_conn[50];
void gprs_at_tcp_conn(int num,unsigned int ip,unsigned short port)
{

	memset(at_cmd_conn,0,50);
#ifdef ONE_CLIENT
	sprintf(at_cmd_conn,"AT+QIOPEN=\"TCP\",\"%d.%d.%d.%d\",%d\r\n",ip>>24,(ip>>16)&0xff,(ip>>8)&0xff,ip&0xff,port);
#else
	sprintf(at_cmd_conn,"AT+QIOPEN=%d,\"TCP\",\"%d.%d.%d.%d\",%d\r\n",num,ip>>24,(ip>>16)&0xff,(ip>>8)&0xff,ip&0xff,port);
#endif	

	//
	gprs_uart_send_string(at_cmd_conn);
	debug(at_cmd_conn);

}




//处理一条at命令返回数据，不带\r\n \0结尾
void one_line_at_cmd_handle(char *pstr,uint16_t len)
{
	uint16_t i,j;

	if(len>GPRS_AT_STR_REV_LEN)
		return;

	for(i=0;i<len;i++) //FIND :  flit param
	{
		if(pstr[i] == ':')
		{
			pstr[i] = 0;
			break;
		}
	}

	for(j=0;j<sizeof(cmd_list_4g)/sizeof(struct _cmd_list);j++)
	{
		if(strcmp((char *)pstr,cmd_list_4g[j].cmd)==0)
		{
			cmd_list_4g[j].func((const char *)&pstr[i+1]);
		}
	}	
}



/*
功能：  1.从环形队列中读取模块返回的AT指令 以及服务器数据 和 ‘>’ 发送标志
		2.解析出模块发出接收到服务器数据的指令后  通过rev_client区分那个链接的数据  rev_len表示应该接收的数据长度
		超时未收到指定长度的数据，所有数据丢弃 退出这个状态
*/
void get_data_from_queue(void)
{
	uint8_t data;
	uint16_t i = 0;
	static uint16_t j = 0;
	uint32_t time_ms;


	if(gprs_str_read_queue(&data) == 0) //成功读出一个字节
	{
		if(gprs_stat.rev_client >= 0) //表示下面的数据都是服务器下发的用户数据，写进app缓冲区
		{
			gprs_data_write_queue(gprs_stat.rev_client, data);
			sprintf(debug_send_buff,"%d\r\n",data);
			debug(debug_send_buff);			
			for(i=1;i<gprs_stat.rev_len;i++) //数据长度
			{
				time_ms = get_ms_count();
				while(gprs_str_read_queue(&data) != 0)//没有数据
				{
					if(get_ms_count()-time_ms>100) //大于100ms 数据出错 丢弃数据
					{
						gprs_stat.rev_client = -1;
						gprs_stat.rev_len = 0;
						gprs_stat.get_data_error_times++;
						return;
					}
				}
				gprs_data_write_queue(gprs_stat.rev_client, data);				
			}
			
			gprs_stat.rev_client = -1;
			gprs_stat.rev_len = 0;
			return;				
		}
		else //进行AT字符串处理
		{
			if(data == '>')  //发送数据
			{
				callback_fun_ready_send(NULL);
			}
			else
			{
				if(data =='\r')
				{
					gprs_str_read_queue(&data);
					if(j==0) //去掉没有命令实体的回车换行符
						return;
					else //缓冲中有完整的命令字符串
					{
						one_line_at_cmd_handle(at_cmd_str_buf,j);
						memset(at_cmd_str_buf,0,GPRS_AT_STR_REV_LEN);
						j=0;						
					}
					
				}
				else
				{
					at_cmd_str_buf[j++] = data;	
					if(j>=GPRS_AT_STR_REV_LEN)	
					{
						memset(at_cmd_str_buf,0,GPRS_AT_STR_REV_LEN);
						j=0;
					}
				}		
			}
			
		}
		
		
	}

	
}

void enable_stop_gprs_mod(uint8_t code)
{
	
}



//异常关闭  保留错误状态码
void error_to_stop_gprs_mod(void)
{
		uint8_t i;
	
		
		grps_power_off();

		gprs_stat.start_enable = 0;
		gprs_stat.stop_sec_count = get_sec_count();  //记录模块启动时间
		gprs_stat.ati_ok = 0;
		gprs_stat.creg_ok = 0;
		gprs_stat.cgreg_ok = 0;	
		gprs_stat.cgatt_ok = 0;
		gprs_stat.csq = 0;   
		gprs_stat.power_status = 0;
		gprs_stat.rdy_count = 0;  //
		gprs_stat.send_at_cmd_times = 0;
		gprs_stat.rev_client = -1;        
		gprs_stat.rev_len = 0;
		gprs_stat.get_data_error_times = 0;
	
		for(i=0;i<CLIENT_COUNT;i++)
		{
			gprs_stat.con_client[i].connect_ok = 0;;	
			gprs_stat.con_client[i].send_setp = 0;
		}

		sprintf(debug_send_buff,"error to stop gprs sec=%d->%d %ds error_code=%d\r\n",gprs_stat.start_sec_count,gprs_stat.stop_sec_count,gprs_stat.stop_sec_count-gprs_stat.start_sec_count,gprs_stat.error_code);
		debug(debug_send_buff);
}


//  主动关闭模块  清除错误状态码
void stop_gprs_mod(void)
{
		uint8_t i;
	
		grps_power_off();

		gprs_stat.error_code = 0;   //主动关闭的时候 清除错误状态码

		gprs_stat.start_enable = 0;
		gprs_stat.stop_sec_count = get_sec_count();  //记录模块启动时间
		gprs_stat.ati_ok = 0;
		gprs_stat.creg_ok = 0;
		gprs_stat.cgreg_ok = 0;
		gprs_stat.cgatt_ok = 0;	
		gprs_stat.csq = 0;   
		gprs_stat.power_status = 0;
		gprs_stat.rdy_count = 0;  //
		gprs_stat.send_at_cmd_times = 0;
		gprs_stat.rev_client = -1;        
		gprs_stat.rev_len = 0;
		gprs_stat.get_data_error_times = 0;
	
		for(i=0;i<CLIENT_COUNT;i++)
		{
			gprs_stat.con_client[i].connect_ok = 0;;	
			gprs_stat.con_client[i].send_setp = 0;
		}

		sprintf(debug_send_buff,"stop gprs sec=%d->%d %ds\r\n",gprs_stat.start_sec_count,gprs_stat.stop_sec_count,gprs_stat.stop_sec_count-gprs_stat.start_sec_count);
		debug(debug_send_buff);
}

void start_gprs_mod(void)
{
		uint8_t i;
	
		grps_power_on();
	
		gprs_stat.start_enable = 1;
		gprs_stat.start_sec_count = get_sec_count();  //记录模块启动时间
		gprs_stat.error_code = 0;
		gprs_stat.error_need_to_reboot = 0;
		gprs_stat.reboot_times++;
		gprs_stat.ati_ok = 0;
		gprs_stat.creg_ok = 0;
		gprs_stat.cgreg_ok = 0;	
		gprs_stat.cgatt_ok = 0;
		gprs_stat.csq = 0;   
		gprs_stat.power_status = 0;
		gprs_stat.rdy_count = 0;  //
		gprs_stat.send_at_cmd_times = 0;
		gprs_stat.rev_client = -1;        
		gprs_stat.rev_len = 0;
		gprs_stat.get_data_error_times = 0;
		
		for(i=0;i<CLIENT_COUNT;i++)
		{
			gprs_stat.con_client[i].connect_ok = 0;;	
			gprs_stat.con_client[i].send_setp = 0;
		}
		
		sprintf(debug_send_buff,"start gprs sec=%d\r\n",gprs_stat.start_sec_count);
		debug(debug_send_buff);
}


//维护gprs模块的状态
void gprs_status_set(void)
{
	
	if(gprs_stat.send_next_at_cmd_time_ms > get_ms_count())  //超时后才会进入
		return;
	

	if(gprs_stat.error_need_to_reboot >0)  //需要复位模块 先检查gprs数据是否发送干净
	{
		if(get_sec_count() - gprs_stat.error_need_to_reboot_sec > 30 || gprs_stat.error_code == GPRS_ERROR_START_ERROR)  //30秒都没法完 认为发送失败
		{
			if(gprs_stat.error_need_to_reboot == 1)
				error_to_stop_gprs_mod();
			else if(gprs_stat.error_need_to_reboot == 2)
				stop_gprs_mod();

			led_ctrl(LED_G,LED_OFF);
			gprs_stat.error_need_to_reboot = 0;		
			return;	
		}
		memset(at_cmd_conn,0,50);
#ifdef ONE_CLIENT		
		sprintf(at_cmd_conn,AT_CMD_AT_QISACK);	
#else
		sprintf(at_cmd_conn,AT_CMD_AT_QISACK,0);
#endif		
		gprs_uart_send_string(at_cmd_conn);	
		gprs_stat.send_next_at_cmd_time_ms = get_ms_count() + 1000;
		return;
	}
	
	if(gprs_stat.start_enable != 1)  //未开机 不做处理
		return;

//	if(gprs_stat.power_status !=1)  //开机模块自动发送一些指令，收到后记录为已开始工作
//	{
//		if(get_sec_count() - gprs_stat.start_sec_count >10) //超时未启动
//		{
//			gprs_stat.error_code = GPRS_ERROR_START_ERROR;
//			goto PWR_OFF_MOD;
//		}
//		return;
//	}


	
	if(gprs_stat.ati_ok == 0)                       
	{
		gprs_stat.send_at_cmd_times++;
		if(gprs_stat.send_at_cmd_times>50)   //5秒
		{
			gprs_stat.error_code = GPRS_ERROR_NO_ATI;
			goto PWR_OFF_MOD;
		}   
		
		gprs_uart_send_string(AT_CMD_ATI);  
		
		gprs_stat.send_next_at_cmd_time_ms = get_ms_count()+100;
		
		return;
	}

	if(gprs_stat.csq <5 || gprs_stat.csq >32)                           
	{

		gprs_stat.send_at_cmd_times++;
		if(gprs_stat.send_at_cmd_times>20)   //20秒
		{
			gprs_stat.error_code = GPRS_ERROR_CSQ_ERROR;
			goto PWR_OFF_MOD;
		}  

		gprs_uart_send_string(AT_CMD_AT_CSQ); 

		gprs_stat.send_next_at_cmd_time_ms = get_ms_count()+1000;  //间隔1秒
		
		return;
	}	
	
	if(gprs_stat.creg_ok == 0)                           
	{
		gprs_stat.send_at_cmd_times++;	
		if(gprs_stat.send_at_cmd_times>20)  //40秒
		{
			gprs_stat.error_code = GPRS_ERROR_CREG_ERROR;
			goto PWR_OFF_MOD;
		}  
	
		gprs_uart_send_string(AT_CMD_AT_CREG); 

		gprs_stat.send_next_at_cmd_time_ms = get_ms_count()+2000;   //间隔2秒
		
		return;
	}	

	if(gprs_stat.cgreg_ok == 0)                           
	{
		gprs_stat.send_at_cmd_times++;	
		if(gprs_stat.send_at_cmd_times>20)  //40秒
		{
			gprs_stat.error_code = GPRS_ERROR_CGREG_ERROR;
			goto PWR_OFF_MOD;
		}  
	
		gprs_uart_send_string(AT_CMD_AT_CGREG); 

		gprs_stat.send_next_at_cmd_time_ms = get_ms_count()+2000;   //间隔2秒
		
		return;
	}	
	
	if(gprs_stat.cgatt_ok == 0)                           
	{
		gprs_stat.send_at_cmd_times++;	
		if(gprs_stat.send_at_cmd_times>2)  //40秒
		{
			gprs_stat.error_code = GPRS_ERROR_CGATT_ERROR;
			goto PWR_OFF_MOD;
		}  
		gprs_uart_send_string(AT_CMD_AT_CGATT); 
		gprs_stat.send_next_at_cmd_time_ms = get_ms_count()+2000;   //间隔2秒
		
		return;
	}		
	
	if(gprs_stat.con_client[0].connect_ok == 0) //未连接
	{
		gprs_stat.send_at_cmd_times++;		
		if(gprs_stat.send_at_cmd_times>3)  //30秒
		{
			gprs_stat.error_code = GPRS_ERROR_CONNECT_ERROR;
			goto PWR_OFF_MOD;
		}  

		gprs_at_tcp_conn(0,gprs_server_ip,gprs_server_port);

		gprs_stat.send_next_at_cmd_time_ms = get_ms_count()+10000;   //间隔10秒
		
		return;		
	}
	else
	{
		if(gprs_stat.con_client[0].send_data_len > 0)  //有数据需要发送
		{
			if(gprs_stat.con_client[0].send_setp == SEND_STEP_NONE) //启动发送
			{
				gprs_stat.send_at_cmd_times++;				
				if(gprs_stat.send_at_cmd_times>3)  //6秒
				{
					gprs_stat.error_code = GPRS_ERROR_WAIT_READY_ERROR;
					goto PWR_OFF_MOD;
				}  

				memset(at_cmd_conn,0,50);
#ifdef	ONE_CLIENT			
				sprintf(at_cmd_conn,AT_CMD_AT_SEND,gprs_stat.con_client[0].send_data_len);
#else
				sprintf(at_cmd_conn,AT_CMD_AT_SEND,0,gprs_stat.con_client[0].send_data_len);				
#endif
				gprs_uart_send_string(at_cmd_conn);	
				debug(at_cmd_conn);
				
				gprs_stat.send_next_at_cmd_time_ms = get_ms_count()+5000;   //间隔2秒	
				
			}
			else if(gprs_stat.con_client[0].send_setp == SEND_STEP_WAIT_SEND_OK) //等待 send ok 超时
			{
				gprs_stat.con_client[0].send_setp = SEND_STEP_NONE;
				gprs_stat.con_client[0].no_send_ok_times++;
				if(gprs_stat.con_client[0].no_send_ok_times>3)  //3次没有收到 send ok   关闭连接
				{
					gprs_stat.con_client[0].send_setp = SEND_STEP_NONE;
					gprs_stat.con_client[0].no_send_ok_times = 0;
					gprs_stat.con_client[0].connect_ok = 0;
					
					gprs_stat.con_client[0].re_connect_to_send_times++;
					if(gprs_stat.con_client[0].re_connect_to_send_times>3)  //3次没有重连接 没有发送成功 关闭模块
					{		
						gprs_stat.con_client[0].re_connect_to_send_times = 0;
						gprs_stat.error_code = GPRS_ERROR_RE_CONNECT_3_NO_SENDOK;
						goto PWR_OFF_MOD;						
					}
					memset(at_cmd_conn,0,50);
#ifdef	ONE_CLIENT					
					sprintf(at_cmd_conn,AT_CMD_AT_CLOSE_CONNECT);
#else					
					sprintf(at_cmd_conn,AT_CMD_AT_CLOSE_CONNECT,0);
#endif					
					gprs_uart_send_string(at_cmd_conn);	
					gprs_stat.send_next_at_cmd_time_ms = get_ms_count()+2000;   //间隔2秒					
				}
			}			
		}
		else //没有数据需要发送
		{
			gprs_stat.send_next_at_cmd_time_ms = get_ms_count()+100;   //间隔100ms

		}
		
 
	}
	
	
	return;
	
	PWR_OFF_MOD:
		gprs_stat.error_need_to_reboot = 1;
		gprs_stat.error_need_to_reboot_sec = get_sec_count();
	
}


/*
功能: 发送一包数据 长度小于1500 ,如果模块未启动  启动模块
注意：该函数只是把数据写入待发送缓冲，底层设备自动发送，可通过查询底层缓冲长度确认是否可以写入
cmd: 

return:  -1, 模块未启动  判断是否执行启动模块: 执行启动返回-1 
		-99  如果模块被异常关闭了  返回-99 不执行启动
		-2  模块已启动   未连接
		-3   数据buf有数据未发送成功
		0 发送成功(只是写入buf成功  等待模块自动发送 发送完后会把len 清零)

*/
int32_t gprs_send_data(uint8_t client,void *pdata,uint16_t len,int32_t *cmd)
{
	int32_t ret;
	
	if(gprs_stat.start_enable == 0) //模块未启动
	{
		if(gprs_stat.reboot_times>1)  //上次被异常关闭
		{
			if(*cmd > 0) //强制重启
			{
				(*cmd)--;
				sprintf(debug_send_buff,"4g_set:force reboot mod times=%d\r\n",gprs_stat.reboot_times);
				debug(debug_send_buff);				
			}
			else
				return -99;  //如果不强制重启模块  就进入休眠模式
		}
		if((get_sec_count()-2) > gprs_stat.stop_sec_count)
			start_gprs_mod();
		return -1;
	}
	
	if(gprs_stat.con_client[client].connect_ok!=1) 
	{
		return -2;
	}

	if(gprs_stat.con_client[0].send_data_len == 0)
	{
		memcpy(gprs_stat.con_client[0].send_data_buff,pdata,len);
		gprs_stat.con_client[0].send_data_len = len;
		sprintf(debug_send_buff,"4g_set:send to buf=%d\r\n",len);
		debug(debug_send_buff);
		ret = 0;
	}
	else
	{
		ret = -3; 
	}

	return ret;
}

void gprs_get_aisack(void)
{
		memset(at_cmd_conn,0,50);
#ifdef ONE_CLIENT		
		sprintf(at_cmd_conn,AT_CMD_AT_QISACK);	
#else
		sprintf(at_cmd_conn,AT_CMD_AT_QISACK,0);
#endif				
		gprs_uart_send_string(at_cmd_conn);		
}


void task_gprs_comm(void)
{
	
	get_data_from_queue();  //处理
	
	gprs_status_set();
}



void callback_fun_at_ok(const char *pstr)
{
	char *start_str;
	if(gprs_stat.ati_ok == 0)
	{
		gprs_stat.ati_ok = 1;
		gprs_stat.send_at_cmd_times = 0;
		gprs_stat.send_next_at_cmd_time_ms = 0;
		start_str = "4g_get:ati ok\r\n";
		gprs_uart_send_string("ATE0\r\n");
	}
	else
		start_str = "4g_get:ok\r\n";
	debug(start_str);		
}


void callback_fun_cgreg(const char *pstr)
{
	
	uint32_t param,param1;
	
	if(pstr[0] != 0)
		sscanf(pstr,"%d,%d",&param,&param1);

	sprintf(debug_send_buff,"4g_get:cgreg=%d %d\r\n",param,param1);
	debug(debug_send_buff);		
	
	if(param1 == 1 || param1 == 5)	
	{
		gprs_stat.cgreg_ok = 1;
		gprs_stat.send_at_cmd_times = 0;
	}
	else
	{
		gprs_uart_send_string(AT_CMD_AT_SET_CGREG);
		debug(AT_CMD_AT_SET_CGREG);	
	}

}


void callback_fun_csq(const char *pstr)
{
	uint32_t param;
	
	if(pstr[0] != 0)
		sscanf(pstr,"%d",&param);

	gprs_stat.csq = param;	
	if(param>10 && param<32)
	{
		gprs_stat.send_at_cmd_times = 0;
		gprs_stat.send_next_at_cmd_time_ms = 0;
		adc_data_param.dev_2g_rssi = param;
		sprintf(debug_send_buff,"4g_get:csq=%d good\r\n",param);
		debug(debug_send_buff);			
	}
	else
	{
		sprintf(debug_send_buff,"4g_get:csq=%d not enough\r\n",param);
		debug(debug_send_buff);		
	}
	

}


void callback_fun_ready_send(const char *pstr)
{
	gprs_stat.send_at_cmd_times = 0;
	gprs_uart_send_bin(gprs_stat.con_client[0].send_data_buff,gprs_stat.con_client[0].send_data_len);
	
	gprs_stat.con_client[0].send_setp = SEND_STEP_WAIT_SEND_OK; //切换等待send ok
	gprs_stat.send_next_at_cmd_time_ms = get_ms_count()+5000;   //5秒超时
	
}

void callback_fun_creg(const char *pstr)
{
	uint32_t param,param1;
	
	if(pstr[0] != 0)
		sscanf(pstr,"%d,%d",&param,&param1);
	
	if(param == 0 && (param1 == 1 || param1 == 5))
	{
		gprs_stat.creg_ok = 1;
		gprs_stat.send_at_cmd_times = 0;
		gprs_stat.send_next_at_cmd_time_ms = 0;		
	}
	
	sprintf(debug_send_buff,"4g_get:creg=%d,%d\r\n",param,param1);
	debug(debug_send_buff);	
}


void callback_fun_rdy(const char *pstr)
{

	gprs_stat.rdy_count++;
	gprs_stat.power_status = 1;
	sprintf(debug_send_buff,"4g_get:RDY times=%d\r\n",gprs_stat.rdy_count);
	debug(debug_send_buff);			
}


void callback_fun_call_ready(const char *pstr)
{
	gprs_stat.power_status = 1;
	char *start_str = "4g_get:call ready\r\n";
	debug(start_str);		
}

void callback_fun_sendok(const char *pstr)
{
	char *start_str = "4g_get:send ok\r\n";
	gprs_stat.con_client[0].send_setp = SEND_STEP_NONE;
	gprs_stat.con_client[0].no_send_ok_times = 0;
	gprs_stat.con_client[0].re_connect_to_send_times = 0;
	gprs_stat.con_client[0].send_data_len = 0;
	gprs_stat.send_next_at_cmd_time_ms = 500;

	debug(start_str);		
}


void callback_fun_qistate(const char *pstr)
{
	
}


void callback_fun_qisack(const char *pstr)
{
	uint32_t send_len,ack_len,noack_len;

	if(pstr[0] != 0)
		sscanf(pstr,"%d,%d,%d",&send_len,&ack_len,&noack_len);

	sprintf(debug_send_buff,"4g_get:send_len=%d ack_len=%d noack_len=%d\r\n",send_len,ack_len,noack_len);
	debug(debug_send_buff);		
	
	if(noack_len == 0)
	{		
		if(gprs_stat.error_need_to_reboot == 1)
			error_to_stop_gprs_mod();		
		wait_2g_adc_data_send_ok();
	}
	
}


void callback_fun_receive(const char *pstr)
{
	uint32_t client,len;

#ifdef ONE_CLIENT	
	if(pstr[0] != 0)
		sscanf(pstr,"%d",&len);
	
	gprs_stat.rev_len = len;
	gprs_stat.rev_client = 0;
	sprintf(debug_send_buff,"4g_get:receive %d len=%d\r\n",client,len);
#else
	if(pstr[0] != 0)
		sscanf(pstr,"%d,%d",&client,&len);
	
	gprs_stat.rev_len = len;
	gprs_stat.rev_client = client;
	sprintf(debug_send_buff,"4g_get:receive %d len=%d\r\n",client,len);
#endif	
	debug(debug_send_buff);	
}


void callback_fun_cereg(const char *pstr)
{
	
}


void callback_fun_0_connect_ok(const char *pstr)
{
	char *start_str = "4g_get:client:0 connect ok\r\n";


	gprs_stat.con_client[0].connect_ok = 1;

	gprs_stat.send_at_cmd_times = 0;
	gprs_stat.send_next_at_cmd_time_ms = 0;	

	debug(start_str);
} 

//+CIPOPEN:SUCCESS,1
void callback_fun_0_connect_fail(const char *pstr)
{
	char *start_str = "4g_get:client:0 connect fail\r\n";
	gprs_stat.send_next_at_cmd_time_ms = 0;
	debug(start_str);	
}  

//:SUCCESS
void callback_fun_0_closed(const char *pstr)
{
	char *start_str = "4g_get:client:0 connect closed\r\n";

	gprs_stat.con_client[0].connect_ok = 0;

	debug(start_str);	
} 

//:SUCCESS,1,20,4
void callback_fun_error(const char *pstr)
{
	char *start_str = "4g_get:error\r\n";

	debug(start_str);
}


void callback_fun_cfun(const char *pstr)
{
	uint32_t param;
	
	if(pstr[0] != 0)
		sscanf(pstr,"%d",&param);
	
	gprs_stat.power_status = 1;
	sprintf(debug_send_buff,"4g_get:cfun=%d\r\n",param);
	debug(debug_send_buff);			
}


void callback_fun_sms_ready(const char *pstr)
{
	gprs_uart_send_string(AT_CMD_AT_QNTP);
	gprs_stat.power_status = 1;
	char *start_str = "4g_get:sms ready\r\n";
	debug(start_str);	
}



void callback_fun_cgatt(const char *pstr)
{
	uint32_t param;
	//uint32_t delay = 10000;
	
	if(pstr[0] != 0)
		sscanf(pstr,"%d",&param);
	
	if(param == 1)
	{
		gprs_stat.cgatt_ok = 1;
		debug("4g_get:+cagtt = 1\r\n");	
		//while(delay--);
		gprs_uart_send_string(AT_CMD_AT_QIMUX); 
		gprs_stat.send_next_at_cmd_time_ms = get_ms_count()+100;   
	}
	else
	{
		gprs_uart_send_string(AT_CMD_AT_SET_CGATT);
		debug("4g_get:+cagtt = 0\r\n");		
		debug(AT_CMD_AT_SET_CGATT);	
		gprs_stat.send_next_at_cmd_time_ms = get_ms_count()+1000;   //间隔0.1秒
	}
}


void callback_fun_qntp(const char *pstr)
{
	uint32_t param;
	
	if(pstr[0] != 0)
		sscanf(pstr,"%d",&param);
	
	if(param == 0)
	{
		gprs_uart_send_string(AT_CMD_AT_CCLK);
		debug("4g_get:qntp ok\r\n");		
		debug(AT_CMD_AT_CCLK);			
		gprs_stat.qntp_ok = 1;
	}	

}


void callback_fun_cclk(const char *pstr)
{
	uint32_t year,month,day,hour,min,sec;
	
	if(pstr[0] != 0)
		sscanf(pstr,"\"%d/%d/%d,%d:%d:%d+",&year,&month,&day,&hour,&min,&sec);
	
	if(1)
	{
		sprintf(debug_send_buff,"4g_get:%d/%d/%d %d:%d:%d\r\n",year,month,day,hour,min,sec);
		debug(debug_send_buff);			
		
	}

}









