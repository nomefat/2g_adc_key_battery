#include "main.h"
#include "rtc.h"
#include "debug_uart.h"
#include "stdio.h"
#include "adc.h"
#include "communication_protocol_handle.h"
#include "work_mode.h"


#define debug(str) copy_string_to_double_buff(str)





extern ADC_HandleTypeDef hadc1;

extern TIM_HandleTypeDef htim4;
	
extern	struct_sys_time sys_time;  //系统时间 包括秒和毫秒

extern struct_adc_data_param adc_data_param; 

extern uint8_t adc_data_buf[1024*32];

extern uint8_t send_data_buf[1500];

struct_adc_data adc_data;





int16_t get_adc_value_bigendian(uint32_t channel);


uint16_t to_big_endian_uint16(uint16_t d)
{
	return ((d>>8) | (d<<8));
}

uint32_t to_big_endian_uint32(uint32_t d)
{
	return (((d>>24) & 0xff) | ((d>>8) & 0xff00) | ((d<<8) & 0xff0000) | ((d<<24) & 0xff000000));
}

uint16_t to_small_endian_uint16(uint16_t d)
{
	return ((d>>8) | (d<<8));
}

uint32_t to_small_endian_uint32(uint32_t d)
{
	return (((d>>24) & 0xff) | ((d>>8) & 0xff00) | ((d<<8) & 0xff0000) | ((d<<24) & 0xff000000));
}




int32_t make_adc_protocal_data(void)
{

	unsigned int samplingRate = 2500; // 采样率
	int samplingCount = 2500; // 采集2500个点

	adc_data_param.dev_sampling_rate = to_big_endian_uint32(2500);

	adc_data.px = (int16_t *)adc_data_buf;
	adc_data.py = (int16_t *)(adc_data_buf+to_small_endian_uint32(adc_data_param.dev_sampling_rate)*2);
	adc_data.pz = (int16_t *)(adc_data_buf+to_small_endian_uint32(adc_data_param.dev_sampling_rate)*4);

    adc_data.len = to_small_endian_uint32(adc_data_param.dev_sampling_rate);
    adc_data.flag = 1;
    adc_data.index = 0;

	if(adc_data.px != NULL && adc_data.py != NULL && adc_data.pz != NULL && adc_data.len != 0 && adc_data.flag == 1)
	{
		HAL_TIM_Base_Start_IT(&htim4);
		HAL_GPIO_WritePin(SENSOR_PWR_CTRL_GPIO_Port,SENSOR_PWR_CTRL_Pin,GPIO_PIN_SET);
		while(adc_data.len > adc_data.index)
		{
			
		}	
    	return 0;	
	}
	else
	{
		return -1;
	}

}



void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == htim4.Instance)
	{
		if(adc_data.index < adc_data.len)
		{
			adc_data.px[adc_data.index] = get_adc_value_bigendian(ADC_CHANNEL_15);
			adc_data.py[adc_data.index] = get_adc_value_bigendian(ADC_CHANNEL_8);
			adc_data.pz[adc_data.index] = get_adc_value_bigendian(ADC_CHANNEL_9);
			//adc_data.px[adc_data.index] = 0x0101;
			//adc_data.py[adc_data.index] = 0x0102;
			//adc_data.pz[adc_data.index] = 0x0103;		
			sprintf(debug_send_buff,"adc_get:%d %d %d\r\n",(adc_data.px[adc_data.index]>>8)|((adc_data.px[adc_data.index]<<8)&0xff00),(adc_data.py[adc_data.index]>>8)|((adc_data.py[adc_data.index]<<8)&0xff00),(adc_data.pz[adc_data.index]>>8)|((adc_data.pz[adc_data.index]<<8)&0xff00));
			debug(debug_send_buff);	
			adc_data.index++;
		}
		else
		{
			HAL_GPIO_WritePin(SENSOR_PWR_CTRL_GPIO_Port,SENSOR_PWR_CTRL_Pin,GPIO_PIN_RESET);
			HAL_TIM_Base_Stop_IT(&htim4);
		}
	}
}


int16_t get_adc_value_bigendian(uint32_t channel)
{
	int16_t ret;
	
	ADC_ChannelConfTypeDef sConfig = {0};

	/** Configure Regular Channel 
	 */
	sConfig.Channel = channel;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
	Error_Handler();
	}

	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1,10); //等待转换完成，第二个参数表示超时时间，单位ms 
	if(HAL_IS_BIT_SET(HAL_ADC_GetState(&hadc1), HAL_ADC_STATE_REG_EOC))
	{
		ret =  HAL_ADC_GetValue(&hadc1);
		ret = (ret>>8) | ((ret<<8)&0xff00);
	} 
	else
	{
		ret =  0;
	}
	return ret;

}









