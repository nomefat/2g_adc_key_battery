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


uint16_t to_big_endian_uint16(uint16_t d);

uint32_t to_big_endian_uint32(uint32_t d);

uint16_t to_small_endian_uint16(uint16_t d);

uint32_t to_small_endian_uint32(uint32_t d);


















#endif

