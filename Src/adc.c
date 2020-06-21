#include "main.h"




extern TIM_HandleTypeDef htim4;
	
	

typedef struct _adc_data
{
	uint8_t flag;
	uint16_t index;
	uint16_t len;
	int16_t *px;
	int16_t *py;
	int16_t *pz;	
}struct_adc_data;


struct_adc_data adc_data;

uint8_t protocal_data_head[32];

uint8_t protocal_data_buf[1024*16];









uint8_t xor_fun(uint8_t *pdata,uint16_t len)
{
	uint16_t i;
	uint8_t ret = 0;
	
	ret = pdata[i];
	for(i=1;i<len;i++)
	{
		ret ^= pdata[i];
	}
}

// ����ѡ�����ָ��̶�12�ֽ�
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
	
		memory[p] = xor_fun(&memory[4],p-4);
		
    memory[0] = p-4;	
}



// ���������ϴ�ָ��䳤:82+6*N�ֽڣ�NΪ����������
// ���ᣬÿ��2048������ʱ���̶�12370�ֽ�
int encodeAccelerationStoreCommand(uint8_t *memory, char *serialNumber, char *model, unsigned int timestamp,
                                   unsigned int samplingRate, int samplingCount,struct_adc_data *p_adc_data) {
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

    // SERIAL_NUMBER = 12λ���� (STRING)
    memory[p++] = 0x01; // SERIAL_NUMBER
    memory[p++] = 0x00;
    memory[p++] = 0x0B; // STRING
    memory[p++] = 0x00; // Serial Number Value
    memory[p++] = 0x00;
    memory[p++] = 0x00;
    memory[p++] = 0x0C;
    for (i = 0; i < 12; i++)
        memory[p++] = serialNumber[i];

    // SERIAL_NUMBER = 6λ�ͺ� (STRING)
    memory[p++] = 0x01; // SERIAL_NUMBER
    memory[p++] = 0x01;
    memory[p++] = 0x0B; // STRING
    memory[p++] = 0x00; // Serial Number Value
    memory[p++] = 0x00;
    memory[p++] = 0x00;
    memory[p++] = 0x06;
    for (i = 0; i < 6; i++)
        memory[p++] = model[i];

    // TIMESTAMP = UNIXʱ�������ȷ���� (UINT32)
    memory[p++] = 0x04; // TIMESTAMP
    memory[p++] = 0x00;
    memory[p++] = 0x06; // UINT32
    memory[p++] = (timestamp >> 24) & 0xFF; // TIMESTAMP Value
    memory[p++] = (timestamp >> 16) & 0xFF;
    memory[p++] = (timestamp >> 8) & 0xFF;
    memory[p++] = (timestamp >> 0) & 0xFF;

    // TEMPERATURE = �¶�ֵ (UINT16)
    memory[p++] = 0x04; // TIMESTAMP
    memory[p++] = 0x01;
    memory[p++] = 0x05; // UINT16
    memory[p++] = 0x00; // ��ʱΪ0
    memory[p++] = 0x00;

    // ACCELERATION_SAMPLING_RATE = �̶�2560������ (UINT32)
    memory[p++] = 0x04; // TIMESTAMP
    memory[p++] = 0x02;
    memory[p++] = 0x06; // UINT32
    memory[p++] = (samplingRate >> 24) & 0xFF; //������ֵ Value
    memory[p++] = (samplingRate >> 16) & 0xFF;
    memory[p++] = (samplingRate >> 8) & 0xFF;
    memory[p++] = (samplingRate >> 0) & 0xFF;

// ACCELERATION_X_ENCODE_CONTENT = ������ (BINARY)
    memory[p++] = 0x04; // ACCELERATION_X_ENCODE_CONTENT
    memory[p++] = 0x03;
    memory[p++] = 0x0A; // BINARY

    memory[p++] = ((2 * samplingCount + 1) >> 24) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 16) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 8) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 0) & 0xFF;

    memory[p++] = 0; // ��ѹ��
		p_adc_data->px = (int16_t *)&memory[p];
    p += samplingCount * 2; // Ԥ���ɼ�����

    // ACCELERATION_X_ENCODE_CONTENT = ������ (BINARY)
    memory[p++] = 0x04; // ACCELERATION_X_ENCODE_CONTENT
    memory[p++] = 0x04;
    memory[p++] = 0x0A; // BINARY

    memory[p++] = ((2 * samplingCount + 1) >> 24) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 16) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 8) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 0) & 0xFF;

    memory[p++] = 0; // ��ѹ��
		p_adc_data->py = (int16_t *)&memory[p];		
    p += samplingCount * 2; // Ԥ���ɼ�����

    // ACCELERATION_X_ENCODE_CONTENT = ������ (BINARY)
    memory[p++] = 0x04; // ACCELERATION_X_ENCODE_CONTENT
    memory[p++] = 0x05;
    memory[p++] = 0x0A; // BINARY

    memory[p++] = ((2 * samplingCount + 1) >> 24) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 16) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 8) & 0xFF;
    memory[p++] = ((2 * samplingCount + 1) >> 0) & 0xFF;

    memory[p++] = 0; // ��ѹ��
		p_adc_data->pz = (int16_t *)&memory[p];		
    p += samplingCount * 2; // Ԥ���ɼ�����
		


		
		//memory[p] = xor_fun(&memory[4],p-4);
		
		p_adc_data->len = samplingCount;
		p_adc_data->flag = 1;
		p_adc_data->index = 0;
		
    memory[0] = (p-4)>>8;	
    memory[1] = p-4;		
}




void make_protocal_data(void)
{
	uint32_t len;
	char *serialNumber = "901200600001";
	char *model = "ZN0111";

	unsigned int timestamp = 1591613193; // Unix ʱ�������ȷ����,UTCʱ�䡣
	unsigned int samplingRate = 2500; // ������
	int samplingCount = 100; // �ɼ�2048����




	encodeAccelerationStoreCommand(protocal_data_buf, serialNumber, model, timestamp, samplingRate, samplingCount,&adc_data);



	if(adc_data.px != NULL && adc_data.px != NULL && adc_data.px != NULL && adc_data.len != 0 && adc_data.flag == 1)
	{
		HAL_TIM_Base_Start_IT(&htim4);
		while(adc_data.len > adc_data.index);
		
		
		
		
	}





}



void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == htim4.Instance)
	{
		if(adc_data.index < adc_data.len)
		{
			adc_data.px[adc_data.index] = 
			adc_data.py[adc_data.index] =
			adc_data.pz[adc_data.index] =
			
			adc_data.index++;
		}
		else
		{
			HAL_TIM_Base_Stop_IT(&htim4);
		}
	}
}


