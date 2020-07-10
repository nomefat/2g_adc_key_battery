#ifndef C_P_H_
#define C_P_H_





#define DEV_SERIAL                "901200600001"
#define DEV_SERIAL_VC            "625E50"

//串口采集数据传输命令字

#define SERVER_COM_GET_ADC_DATA                 0x00a0            //串口索要adc采集数据
#define DEV_COM_SEND_ADC_DATA                   0x01a0            //节点发送adc采集数据
#define SERVER_COM_GET_N_PACKET_ADC_DATA        0x02a0            //串口索要指定包序号的adc数据



//2G 自测包定义
#define DEV_2G_SEND_SELF_TEST_ADC_DATA          0X0001             //设备发送自测adc采集数据
#define SERVER_SEND_SELF_TEST_OK                0X0101             //设备发送自测adc采集数据

//2G 激活包定义
#define DEV_2G_SEND_ACTIVATE_PACKET             0X0002             //设备发送激活请求包
#define SERVER_2G_SEND_ACTIVATE_FAILD           0X0102             //设备发送激活请求包
#define SERVER_2G_SEND_ACTIVATE_OK              0X0202             //设备发送激活请求包

//2G 下发算法参数包定义
#define SERVER_2G_SEND_SF_CFG                   0x0003              //服务器下发算法参数

//2G 上传adc采集数据
#define DEV_2G_SEND_ADC_DATA                    0x0000             //设备发送adc采集数据
#define SERVER_2G_GET_N_PACKET_ADC_DATA         0x0100              //服务器索要第N包adc采集数据
#define SERVER_2G_SEND_ADC_DATA_OK              0x0200              //服务器索要第N包adc采集数据

//2G 服务器下发节点参数
#define SERVER_2G_SEND_CFG_DATA                 0x0004              //服务器发送配置包
#define DEV_2G_SEND_CFG_DATA_OK                 0x0104              //设备发送配置包接收完成

//2G 服务器启动固件更新
#define SERVER_2G_START_FIRMWARE_UPDATA         0x0500              //服务器启动远程更新固件
#define DEV_2G_GET_N_PACKET_UPDATA              0x0600              //设备索要第N地址的固件包
#define SERVER_2G_ACK_N_PACKET_FIRMWARE         0x0700              //服务器下发第N地址的固件包
#define DEV_2G_SEND_FIRMWARE_UPDATA_OK          0x0800              //设备发送升级固件接收完成



//adc采集包的类型
#define ADC_DATA_TYPE_SELF_TEST                1        //检测模式
#define ADC_DATA_TYPE_AUTO_STUDY               2        //学习模式
#define ADC_DATA_TYPE_HEART                    3        //心跳包
#define ADC_DATA_TYPE_START_STOP               4        //启停包
#define ADC_DATA_TYPE_BLUE                     5        //蓝牙包







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
    uint8_t dev_work_mode;           //工作模式
    uint32_t dev_send_count;        //累计次数
    uint8_t data_type;             //数据包类型
    uint8_t dev_battery;            //电池电压
    uint8_t dev_2g_rssi;            //
    uint8_t dev_check_start_or_stop;   
    uint32_t dev_sampling_rate;     
    uint16_t temperature;
}struct_adc_data_param;                   //adc数据块的一些参数 

typedef struct _protocol_adc_data
{
    struct_protocol_head head;
    uint8_t dev_serial[12];
    uint8_t verification_code[6];       //验证码
    uint32_t dev_get_adc_data_time; 
    uint16_t packet_count;
    uint16_t packet_index;    
}struct_protocol_adc_data;               //adc数据的协议


typedef struct _version
{
    uint8_t v_h;
    uint8_t v_m;
    uint8_t v_l;    
}struct_version;

typedef struct _activate_packet_data      //激活申请包
{
    struct_protocol_head head;   
    int8_t dev_serial[12];
    uint8_t verification_code[6];
    int8_t dev_model[6];
    uint32_t date_of_made_sec;
    uint8_t made_in_factory;
    int8_t m26_ccid[20];
    uint32_t sim_card_activate_time_sec;
    uint32_t sim_card_timeout_sec;
    uint8_t vibration_sensor_type[16];        //振动传感器类型
    uint32_t vibration_sensor_pinxiang;  //振动传感器频响
    struct_version dev_version;          //节点版本
    struct_version lixinbeng_version;    //离心泵算法版本
    struct_version wangfubeng_version;  //往复泵算法版本
    struct_version motor_version;       //电机算法版本
    uint16_t crc;
}struct_activate_packet_data;


typedef struct _activate_packet_ack             //激活ack包
{
    uint32_t server_time_sec; //校准时间
    uint16_t band_dev_type;   //绑定设备类型
    uint16_t auto_study_send_packet_count; //自动学习要上传的包数
    uint16_t timer_send_adc_data; //定时发送adc采集数据的时间
}struct_activate_packet_ack;










#pragma pack()



extern uint8_t adc_data_buf[1024*32];

extern uint8_t rev_data_buf[1500];

extern uint8_t send_data_buf[1500];



//int encodeServiceSelectCommand(char *memory) ;

//int encodeAccelerationStoreCommand(uint8_t *memory, char *serialNumber, char *model, unsigned int timestamp,
//                                   unsigned int samplingRate, int samplingCount,struct_adc_data *p_adc_data) ;


uint8_t xor_fun(uint8_t *pdata,uint16_t len);

uint16_t MB_CRC16(uint8_t *_pushMsg,uint16_t _usDataLen);

void handle_2g_progocol(void *pdata);



void send_to_server_activate_data(void);













#endif
