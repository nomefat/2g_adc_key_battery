#ifndef C_P_H_
#define C_P_H_









#define TAG_CMD                 0X0000
#define TAG_SERVICE_ID          0X0100
#define TAG_PING                0X0200
#define TAG_PONG                0X0300



#define TAG_YWZL                                0X00E0 //业务指令
#define TAG_NODE_SERIAL_NUMBER                  0X0001 //节点串号
#define TAG_NODE_TYPE_NUMBER                    0X0101 //节点型号
#define TAG_GCDM                                0X0301 //工厂代码
#define TAG_MAKE_DATETIME                       0x0401 //生产日期
#define TAG_SIM_LIVE_DATETIME                   0X0501 //SIM卡激活时间
#define TAG_SIM_EXPIRE_DATETIME                 0X0601 //SIM卡失效时间















#pragma pack(1)


typedef struct _data_set
{
    uint16_t tag;
    uint8_t vr;
    void *pdata;
}struct_data_set;





#pragma pack()



extern uint8_t protocal_data_head[32];

extern uint8_t protocal_data_buf[1024*16];



int encodeServiceSelectCommand(char *memory) ;

int encodeAccelerationStoreCommand(uint8_t *memory, char *serialNumber, char *model, unsigned int timestamp,
                                   unsigned int samplingRate, int samplingCount,struct_adc_data *p_adc_data) ;


uint8_t xor_fun(uint8_t *pdata,uint16_t len);


#endif
