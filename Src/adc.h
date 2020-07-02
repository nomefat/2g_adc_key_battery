#ifndef ADC_H_
#define ADC_H_










typedef struct _adc_data
{
	uint8_t flag;
    volatile uint16_t index;
    volatile  uint16_t len;
	int16_t *px;
	int16_t *py;
	int16_t *pz;	
}struct_adc_data;








int32_t make_adc_protocal_data(void);





















#endif

