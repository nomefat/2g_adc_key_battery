
#include "main.h"
#include "rtc.h"
#include "debug_uart.h"
#include "stdio.h"
#include "adc.h"
#include "communication_protocol_handle.h"




uint8_t protocal_data_head[32];

uint8_t protocal_data_buf[1024*16];


int8_t * p_int8;
int16_t * p_int16;
int32_t * p_int32;
uint8_t * p_uint8;
uint16_t * p_uint16;
uint32_t * p_uint32;
float * p_float32;
double * p_float64;
uint8_t * p_bin;
int8_t * p_string;




void *VR_data_ptr[10];





uint8_t xor_fun(uint8_t *pdata,uint16_t len)
{
	uint16_t i = 0;
	uint8_t ret = 0;
	
	
	for(i=0;i<len;i++)
	{
		ret ^= pdata[i];
	}
	return ret;
}


// 编码选择服务指令，固定12字节
int encodeServiceSelectCommand(char *memory) 
{
    int p = 0;

    //len	
    memory[p++] = 0x00;
    memory[p++] = 0x00;	
    memory[p++] = 0x00;
    memory[p++] = 0x00;
	
    // DataSet count = 2;
    memory[p++] = 0x00;
    memory[p++] = 0x02;

    // COMMAND = NOTICE (UINT8)
    memory[p++] = 0x00; // COMMAND
    memory[p++] = 0x00;
    memory[p++] = 0x05; // UINT16
    memory[p++] = 0x02; // NOTICE
    memory[p++] = 0x00;
	
    // SERVICE_ID = ASTROLOGY_ACCELERATION_STORE (UINT16)
    memory[p++] = 0x00; // SERVICE_ID
    memory[p++] = 0x01;
    memory[p++] = 0x05; // UINT16
    memory[p++] = 0xF0; // ASTROLOGY_ACCELERATION_STORE
    memory[p++] = 0x00;
	
		memory[p] = xor_fun((uint8_t *)&memory[4],p-4);
		
    memory[3] = p-4;	
		
		return 0;
}



// 编码数据上传指令，变长:82+6*N字节，N为采样点数。
// 三轴，每轴2048采样点时，固定12370字节
int encodeAccelerationStoreCommand(uint8_t *memory, char *serialNumber, char *model, unsigned int timestamp,
                                   unsigned int samplingRate, int samplingCount,struct_adc_data *p_adc_data) 
{
    int p = 0;
		int i = 0;
		
    memory[p++] = 0x00;
    memory[p++] = 0x00;	
    memory[p++] = 0x00;
    memory[p++] = 0x00;
																		 
    // iod keys = 9
    memory[p++] = 0x00;
    memory[p++] = 0x09;

    // COMMAND = NOTICE (UINT8)
    memory[p++] = 0x00; // COMMAND
    memory[p++] = 0x00;
    memory[p++] = 0x05; // UINT16
    memory[p++] = 0x00; // STORE
    memory[p++] = 0x00;

    // SERIAL_NUMBER = 12位串号 (STRING)
    memory[p++] = 0x01; // SERIAL_NUMBER
    memory[p++] = 0x00;
    memory[p++] = 0x0B; // STRING
    memory[p++] = 0x00; // Serial Number Value
    memory[p++] = 0x00;
    memory[p++] = 0x00;
    memory[p++] = 0x0C;
    for (i = 0; i < 12; i++)
        memory[p++] = serialNumber[i];

    // SERIAL_NUMBER = 6位型号 (STRING)
    memory[p++] = 0x01; // SERIAL_NUMBER
    memory[p++] = 0x01;
    memory[p++] = 0x0B; // STRING
    memory[p++] = 0x00; // Serial Number Value
    memory[p++] = 0x00;
    memory[p++] = 0x00;
    memory[p++] = 0x06;
    for (i = 0; i < 6; i++)
        memory[p++] = model[i];

    // TIMESTAMP = UNIX时间戳，精确到秒 (UINT32)
    memory[p++] = 0x04; // TIMESTAMP
    memory[p++] = 0x00;
    memory[p++] = 0x06; // UINT32
    memory[p++] = (timestamp >> 24) & 0xFF; // TIMESTAMP Value
    memory[p++] = (timestamp >> 16) & 0xFF;
    memory[p++] = (timestamp >> 8) & 0xFF;
    memory[p++] = (timestamp >> 0) & 0xFF;

    // TEMPERATURE = 温度值 (INT16)
    memory[p++] = 0x04; // TIMESTAMP
    memory[p++] = 0x01;
    memory[p++] = 0x02; // INT16
    memory[p++] = 0x00; // 暂时为0
    memory[p++] = 0x00;

    // ACCELERATION_SAMPLING_RATE = 固定2560采样率 (UINT32)
    memory[p++] = 0x04; // TIMESTAMP
    memory[p++] = 0x02;
    memory[p++] = 0x06; // UINT32
    memory[p++] = (samplingRate >> 24) & 0xFF; //采样率值 Value
    memory[p++] = (samplingRate >> 16) & 0xFF;
    memory[p++] = (samplingRate >> 8) & 0xFF;
    memory[p++] = (samplingRate >> 0) & 0xFF;

// ACCELERATION_X_ENCODE_CONTENT = 振动数据 (BINARY)
    memory[p++] = 0x04; // ACCELERATION_X_ENCODE_CONTENT
    memory[p++] = 0x03;
    memory[p++] = 0x0A; // BINARY

    memory[p++] = ((2 * samplingCount + 1) >> 24) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 16) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 8) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 0) & 0xFF;

    memory[p++] = 0; // 无压缩
		p_adc_data->px = (int16_t *)&memory[p];
    p += samplingCount * 2; // 预留采集数据

    // ACCELERATION_X_ENCODE_CONTENT = 振动数据 (BINARY)
    memory[p++] = 0x04; // ACCELERATION_X_ENCODE_CONTENT
    memory[p++] = 0x04;
    memory[p++] = 0x0A; // BINARY

    memory[p++] = ((2 * samplingCount + 1) >> 24) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 16) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 8) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 0) & 0xFF;

    memory[p++] = 0; // 无压缩
		p_adc_data->py = (int16_t *)&memory[p];		
    p += samplingCount * 2; // 预留采集数据

    // ACCELERATION_X_ENCODE_CONTENT = 振动数据 (BINARY)
    memory[p++] = 0x04; // ACCELERATION_X_ENCODE_CONTENT
    memory[p++] = 0x05;
    memory[p++] = 0x0A; // BINARY

    memory[p++] = ((2 * samplingCount + 1) >> 24) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 16) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 8) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 0) & 0xFF;

    memory[p++] = 0; // 无压缩
		p_adc_data->pz = (int16_t *)&memory[p];		
    p += samplingCount * 2; // 预留采集数据
		


		
		//memory[p] = xor_fun(&memory[4],p-4);
		
    p_adc_data->len = samplingCount;
    p_adc_data->flag = 1;
    p_adc_data->index = 0;
    
    memory[2] = (p-4)>>8;	
    memory[3] = p-4;		
		
		return 0;
}











// 进行协议解析
void protocol_data_handle(void *pdata,uint16_t len)
{
    uint16_t i = 0;
    uint16_t data_index = 0;
    uint16_t data_set_count;
		struct_data_set *p_data_set;
		uint8_t *p = pdata;
	
		VR_data_ptr[0] = p_int8;
		VR_data_ptr[1] = p_int16;
		VR_data_ptr[2] = p_int32;
		VR_data_ptr[3] = p_uint8;
		VR_data_ptr[4] = p_uint16;
		VR_data_ptr[5] = p_uint32;
		VR_data_ptr[6] = p_float32;
		VR_data_ptr[7] = p_float64;
		VR_data_ptr[8] = p_bin;
		VR_data_ptr[9] = p_string;	

    data_set_count = (p[0]<<8) + ((uint8_t *)pdata)[1] ;

    data_index  = 2;
    for(i=0;i<data_set_count;i++)
    {
        p_data_set = (struct_data_set *)(&p[data_index]);
        switch(p_data_set->tag)
        {
					case TAG_CMD:
						break;
        }
        
    }
}






















