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



//串口采集数据传输命令字

#define SERVER_COM_GET_ADC_DATA                 0x00a0            //串口索要adc采集数据

#define DEV_COM_SEND_ADC_DATA                   0x01a0            //节点发送adc采集数据

#define SERVER_COM_GET_N_PACKET_ADC_DATA        0x02a0            //串口索要指定包序号的adc数据


//2G采集数据传输命令字

#define DEV_2G_SEND_ADC_DATA                    0x0000             //设备发送adc采集数据

#define SERVER_2G_GET_N_PACKET_ADC_DATA         0x0100              //服务器索要第N包adc采集数据

#define SERVER_2G_SEND_CFG_DATA                 0x0200              //服务器发送配置包

#define DEV_2G_SEND_CFG_DATA_OK                 0x0400              //设备发送配置包接收完成

#define SERVER_2G_START_FIRMWARE_UPDATA         0x0500              //服务器启动远程更新固件

#define DEV_2G_GET_N_PACKET_UPDATA              0x0600              //设备索要第N地址的固件包

#define SERVER_2G_ACK_N_PACKET_FIRMWARE         0x0700              //服务器下发第N地址的固件包

#define DEV_2G_SEND_FIRMWARE_UPDATA_OK          0x0800              //设备发送升级固件接收完成

#define SERVER_2G_SEND_ADC_DATA_OK              0x0900              //服务器发送此次服务完成





#pragma pack(1)


typedef struct _data_set
{
    uint16_t tag;
    uint8_t vr;
    void *pdata;
}struct_data_set;





typedef struct _protocol_head
{
    uint16_t sid_cmd;
    uint32_t len;
    uint8_t len_xor;
}struct_protocol_head;


typedef struct _param_and_adc_data
{
    uint8_t dev_serial[12];
    uint8_t dev_work_mode;
    uint8_t dev_battery;
    uint8_t dev_2g_rssi;  
    uint8_t dev_check_start_or_stop;  
    uint32_t dev_get_adc_data_time;  
    uint32_t dev_sampling_rate;     
    uint16_t temperature;
}struct_adc_data_param;                   //adc数据块的一些参数 

typedef struct _protocol_adc_data
{
    struct_protocol_head head;
    uint16_t packet_count;
    uint16_t packet_index;    
}struct_protocol_adc_data;               //adc数据的协议

















#pragma pack()



extern uint8_t adc_data_buf[1024*32];

extern uint8_t rev_data_buf[1500];

extern uint8_t send_data_buf[1500];



//int encodeServiceSelectCommand(char *memory) ;

//int encodeAccelerationStoreCommand(uint8_t *memory, char *serialNumber, char *model, unsigned int timestamp,
//                                   unsigned int samplingRate, int samplingCount,struct_adc_data *p_adc_data) ;


uint8_t xor_fun(uint8_t *pdata,uint16_t len);

uint16_t MB_CRC16(uint8_t *_pushMsg,uint8_t _usDataLen);

void handle_2g_progocol(void *pdata);

















#endif
