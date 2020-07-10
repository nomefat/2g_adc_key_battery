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
#include "communication_protocol_handle.h"
#include "work_mode.h"
#include "adc.h"






#define debug(str) copy_string_to_double_buff(str)


int gprs_data_queue_write[CLIENT_COUNT] = {0};               
int gprs_data_queue_read[CLIENT_COUNT] = {0};

uint8_t gprs_data_buff_client[CLIENT_COUNT][GPRS_DATA_BUFF_LENGH]; 

uint8_t task_send_gprs_data_enable = 0;



struct_gprs_task_manage gprs_task_manage;

extern uint8_t rev_data_buf[];

extern uint8_t adc_data_buf[];

extern uint8_t send_data_buf[];

extern struct_protocol_adc_data protocol_adc_data;               //adc���ݵ�Э��,��¼�������͵�ǰ����

extern struct_protocol_adc_data protocol_adc_data_s_get;               //adc���ݵ�Э��, ��¼��������Ҫ�İ����

extern struct_adc_data_param adc_data_param;  //adc���ݿ��һЩ���� 


extern uint8_t bule_switch;  //�����ر�


void btn_handle(void);

void task_2g_rev_data_handle(void);


/*
 * 
 * 
 * 
*/
int8_t gprs_data_write_queue(uint8_t c,uint8_t data)
{
    if(c>=CLIENT_COUNT)
        return -2;

	if((gprs_data_queue_write[c]+1)%GPRS_STR_QUEUE_LENGH==gprs_data_queue_read[c])
		return -1;

	gprs_data_buff_client[c][gprs_data_queue_write[c]] = data;
	gprs_data_queue_write[c] = (gprs_data_queue_write[c]+1)%GPRS_DATA_BUFF_LENGH;

	return 0;
}

/*
 * 
 * 
 * 
*/
int8_t gprs_data_read_queue(uint8_t c,uint8_t *pdata)
{
    if(c>=CLIENT_COUNT)
        return -2;  

	if(gprs_data_queue_write[c]==gprs_data_queue_read[c])
		return -1;	

	*pdata = gprs_data_buff_client[c][gprs_data_queue_read[c]];
	gprs_data_queue_read[c] = (gprs_data_queue_read[c]+1)%GPRS_STR_QUEUE_LENGH;
	return 0;
}




void task_gprs_app(void)
{
	
	task_send_gprs_data();	
	
	task_2g_rev_data_handle();

	btn_handle();
	
}




void task_send_gprs_data(void)
{
	static uint32_t ms = 0;
	static uint16_t len;
	int32_t ret;
	uint16_t crc,packet_len;
	uint32_t dev_sampling_rate;

	
	if(ms > get_ms_count())
		return;

	if(gprs_task_manage.send_activate_data_enable == ENABLE_SEND)
	{
		ret = gprs_send_data(0,send_data_buf,sizeof(struct_activate_packet_data),&gprs_task_manage.error_to_reboot_mod_times);
		if(ret == 99)  //��������
		{

		}	
		else if(ret == -1)
		{
			ms = get_ms_count() + 1000;
		}	
		else if(ret == -3 || ret == -2)
		{
			ms = get_ms_count() + 100;				
		}			
		else if(ret == 0) //д��buf
		{
			gprs_task_manage.send_activate_data_enable = SEND_OK;
		}
	}
	else if(gprs_task_manage.send_adc_data_enable == 1)  //��ѯ���з���adc���ݵ�����
	{	
		if(gprs_task_manage.task_setp == TASK_NONE) //��һ�� ׼��adc����
		{
			if(make_adc_protocal_data() == 0)
			{
				gprs_task_manage.task_setp = SEND_ADC_DATA_ING;
			}
			else
			{
				//adc����׼������
				return;
			}
			
		}	
		if(gprs_task_manage.task_setp == SEND_ADC_DATA_ING)
		{
			if(protocol_adc_data.packet_count == 0)
			{
				memcpy(protocol_adc_data.dev_serial,DEV_SERIAL,12);
				memcpy(protocol_adc_data.verification_code,DEV_SERIAL_VC,6);
				protocol_adc_data.dev_get_adc_data_time = to_big_endian_uint32(make_unix_sec(sys_time.sDate, sys_time.sTime));
				protocol_adc_data.head.sid_cmd = gprs_task_manage.sid_cmd;
				protocol_adc_data.head.len = to_big_endian_uint32(26 + sizeof(adc_data_param) + 1024); 
				protocol_adc_data.head.len_xor = xor_fun((uint8_t *)&(protocol_adc_data.head.len),4); 

				adc_data_param.data_type = gprs_task_manage.adc_data_type;
				adc_data_param.dev_battery = 185;
				adc_data_param.dev_work_mode = work_mode.mode+1;
				adc_data_param.dev_send_count = to_big_endian_uint32(work_mode.dev_send_count++);
				

				dev_sampling_rate = to_small_endian_uint32(adc_data_param.dev_sampling_rate);
				if((dev_sampling_rate*6)%1024 == 0)
					protocol_adc_data.packet_count =  to_big_endian_uint16((dev_sampling_rate*6)/1024);
				else
				{
					protocol_adc_data.packet_count =  to_big_endian_uint16((dev_sampling_rate*6)/1024 + 1);
				}
				protocol_adc_data.packet_index = 0;
				len = sizeof(protocol_adc_data);
				memcpy(send_data_buf,&protocol_adc_data,len);
				memcpy(&send_data_buf[len],&adc_data_param,sizeof(adc_data_param));
				len += sizeof(adc_data_param);
				memcpy(&send_data_buf[len],&adc_data_buf[to_small_endian_uint16(protocol_adc_data.packet_index)*1024],1024);
				len += 1024;
				crc = MB_CRC16(send_data_buf,len);
				send_data_buf[len++] = ((crc>>8) & 0xff);
				send_data_buf[len++] = (crc & 0xff);
			}
			if(len>0)
			{
				ret = gprs_send_data(0,send_data_buf,len,&gprs_task_manage.error_to_reboot_mod_times);
				if(ret == 99)  //��������
				{

				}	
				else if(ret == -1)
				{
					ms = get_ms_count() + 1000;
				}	
				else if(ret == -3 || ret == -2)
				{
					ms = get_ms_count() + 100;				
				}					
				else if(ret == 0) //д��buf  ���Լ���׼��д����һ����
				{
					
					ms = get_ms_count() + 100;	
					if(gprs_task_manage.task_setp == SEND_ADC_DATA_ING)
					{
						protocol_adc_data.packet_index = to_big_endian_uint16(to_small_endian_uint16(protocol_adc_data.packet_index) + 1);
						
						if(to_small_endian_uint16(protocol_adc_data.packet_index) >= to_small_endian_uint16(protocol_adc_data.packet_count)) //�������
						{
							gprs_task_manage.task_setp = WAIT_PROCOTAL_SEND_OK;
							memset(&protocol_adc_data,0,sizeof(protocol_adc_data));	
							len = 0;						
							return;
						}
						
						dev_sampling_rate = to_small_endian_uint32(adc_data_param.dev_sampling_rate);
										
						if(to_small_endian_uint16(protocol_adc_data.packet_index)+1 == to_small_endian_uint16(protocol_adc_data.packet_count)) //���һ��
							packet_len = (dev_sampling_rate*6)%1024;
						else
							packet_len = 1024;
						
						protocol_adc_data.head.len = to_big_endian_uint32(26 + packet_len); 
						protocol_adc_data.head.len_xor = xor_fun((uint8_t *)&(protocol_adc_data.head.len),4); 
						
						len = sizeof(protocol_adc_data);
						memcpy(send_data_buf,&protocol_adc_data,len);
						memcpy(&send_data_buf[len],&adc_data_buf[to_small_endian_uint16(protocol_adc_data.packet_index)*1024],packet_len);
						len += packet_len;
						crc = MB_CRC16(send_data_buf,len);
						send_data_buf[len++] = ((crc>>8) & 0xff);
						send_data_buf[len++] = (crc & 0xff);
					}	
				}
			}
			else
			{

			}						
		}
		else if(gprs_task_manage.task_setp == WAIT_PROCOTAL_SEND_OK) //�ȴ�����OK
		{
				gprs_get_aisack();
				ms = get_ms_count() + 1000;
		}
	}
}



//��������0���յ��ĵ�����
void task_2g_rev_data_handle(void)
{
	uint8_t data;
	static uint16_t index = 0;
	static uint8_t len_xor_flag = 0;
	uint16_t packet_crc;
	struct_protocol_head *p_protocol_head = (struct_protocol_head *)rev_data_buf;

	for(;;)
	{
		if(gprs_data_read_queue(0,&data) == 0)
		{
			rev_data_buf[index++] = data;
			if(index >=9)  //��С�������ݰ�������9
			{
				if(len_xor_flag == 0)  //len����δУ��
				{
					if(xor_fun((uint8_t *)&(p_protocol_head->len),4) != p_protocol_head->len_xor) //���ȵ�У����� ����һ����ȷ�İ�
					{
						index--;
						memcpy(rev_data_buf,&rev_data_buf[1],index);
						break;
					}
					else
					{
						len_xor_flag = 1;
					}
				}
				if(len_xor_flag == 1)  //��һ��У���Ѿ�ͨ��  �ȴ�����crcУ��
				{
					if(index >= (to_small_endian_uint32(p_protocol_head->len)+9)) //��������Ҫ��
					{
						packet_crc = (rev_data_buf[index-1]) + (rev_data_buf[index-2]<<8);
						if(packet_crc == MB_CRC16(rev_data_buf,to_small_endian_uint32(p_protocol_head->len)+7)) //У��ͨ��
						{
							handle_2g_progocol(rev_data_buf);							
						}
						else //У��ʧ��
						{
							/* code */
						}	
						len_xor_flag = 0;					
						index = 0;
					}
				}	
			}
		}
		else
		{
			break;
		}
		
	}
}




void start_2g_send_adc_data_task(uint16_t sid_cmd,uint8_t adc_data_type)
{
	gprs_task_manage.send_adc_data_enable = 1;
	gprs_task_manage.sid_cmd = sid_cmd;
	gprs_task_manage.adc_data_type = adc_data_type;
	gprs_task_manage.task_setp = 0;
	gprs_task_manage.make_adc_error_times = 0;
	gprs_task_manage.error_to_reboot_mod_times = 300;
}

void wait_2g_adc_data_send_ok(void)
{
	if(gprs_task_manage.send_adc_data_enable == 1)
	{
		if(gprs_task_manage.task_setp == WAIT_PROCOTAL_SEND_OK)
		{
			gprs_task_manage.send_adc_data_enable = 2;
			gprs_task_manage.task_setp = 0;
			gprs_task_manage.make_adc_error_times = 0;
			gprs_task_manage.error_to_reboot_mod_times = 3;	
			sprintf(debug_send_buff,"4g_set:send adc data ok len=%d\r\n",adc_data_param.dev_sampling_rate*6);
			debug(debug_send_buff);				
		}
	}
}

int8_t get_2g_send_adc_data_stat(void)
{
	return gprs_task_manage.send_adc_data_enable;
}

int8_t get_2g_send_activate_data_stat(void)
{
	return gprs_task_manage.send_activate_data_enable;
}












