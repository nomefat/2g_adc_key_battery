#include "main.h"
#include "rtc.h"
#include "debug_uart.h"
#include "stdio.h"
#include "adc.h"
#include "communication_protocol_handle.h"
#include "work_mode.h"
#include "gprs_app.h"
#include "stdio.h"
#include "string.h"

const struct_version dev_version = {0,0,1};




uint8_t adc_data_buf[1024*32];

uint8_t rev_data_buf[1500];

uint8_t send_data_buf[1500];

struct_adc_data_param adc_data_param;  //adc数据块的一些参数 

struct_protocol_adc_data protocol_adc_data;               //adc数据的协议,记录包总数和当前包数

struct_protocol_adc_data protocol_adc_data_s_get;               //adc数据的协议, 记录服务器索要的包序号

struct_activate_packet_data activate_packet_data;

struct_activate_packet_ack ctivate_packet_ack;



extern struct_gprs_task_manage gprs_task_manage;




void send_to_server_activate_data(void)
{
  struct_activate_packet_data *p_activate_packet_data = (struct_activate_packet_data *)send_data_buf;

  memcpy(p_activate_packet_data->dev_serial,DEV_SERIAL,12);
  p_activate_packet_data->head.sid_cmd = DEV_2G_SEND_ACTIVATE_PACKET;
  p_activate_packet_data->head.len = to_big_endian_uint32(sizeof(struct_activate_packet_data) - 9);
  p_activate_packet_data->head.len_xor = xor_fun((uint8_t *)&(p_activate_packet_data->head.len),4);

  p_activate_packet_data->dev_version = dev_version;
  memcpy(p_activate_packet_data->verification_code,DEV_SERIAL_VC,6);

  memcpy(p_activate_packet_data->dev_model,DEV_SERIAL_VC,6);	
	
  memcpy(p_activate_packet_data->m26_ccid,"12345678901234567890",20);	

  memcpy(p_activate_packet_data->vibration_sensor_type,"1234567890123456",16);	  	
	
  p_activate_packet_data->crc = to_big_endian_uint16(MB_CRC16(send_data_buf,sizeof(struct_activate_packet_data) - 2));

  gprs_task_manage.send_activate_data_enable = ENABLE_SEND;

}





void handle_2g_progocol(void *pdata)
{
  struct_protocol_head *p_protocol_head = (struct_protocol_head *)pdata; 
  uint8_t *protocol_data = &((uint8_t *)pdata)[7];

  switch(p_protocol_head->sid_cmd)
  {
    case SERVER_SEND_SELF_TEST_OK:  //自测OK
      get_server_self_test_ok();
      break;
    case SERVER_2G_SEND_ACTIVATE_FAILD :  //拒绝激活
      get_server_activate_faild();
      break;
    case SERVER_2G_SEND_ACTIVATE_OK :  //激活成功
      get_server_activate_ok(protocol_data);
      break;

    case SERVER_2G_GET_N_PACKET_ADC_DATA:     ;     //服务器索要adc采集数据
      break;

    case SERVER_2G_SEND_CFG_DATA:     ;       //服务器发送配置包
      break;

//    case SERVER_2G_SEND_CFG_DATA:     ;       //服务器发送配置包
//      break;


    case SERVER_2G_START_FIRMWARE_UPDATA :      ;       //服务器启动远程更新固件
      break;


    case SERVER_2G_ACK_N_PACKET_FIRMWARE:    ;          //服务器下发第N地址的固件包
      break;


    case SERVER_2G_SEND_ADC_DATA_OK:         ;           //服务器发送此次服务完成
			get_server_activate_ok_adc_data_ack();
      break;

    default : ; break;
  }

}
















// CRC 高位字节值表
static const uint8_t auchCRCHi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
} ;
// CRC 低位字节值表
static const uint8_t auchCRCLo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
    0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
    0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
    0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
    0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
    0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
    0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
    0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
    0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
    0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
    0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
    0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
    0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
    0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
    0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
    0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

/* 函数体 --------------------------------------------------------------------*/
/**
  * 函数功能: Modbus CRC16 校验计算函数
  * 输入参数: pushMsg:待计算的数据首地址,usDataLen:数据长度
  * 返 回 值: CRC16 计算结果
  * 说    明: 计算结果是高位在前,需要转换才能发送
  */
uint16_t MB_CRC16(uint8_t *_pushMsg,uint16_t _usDataLen)
{
  uint8_t uchCRCHi = 0xFF;
  uint8_t uchCRCLo = 0xFF;
  uint16_t uIndex;
  while(_usDataLen--)
  {
    uIndex = uchCRCLo ^ *_pushMsg++;
    uchCRCLo = uchCRCHi^auchCRCHi[uIndex];
    uchCRCHi = auchCRCLo[uIndex];
  }
  return (uchCRCHi<<8|uchCRCLo);
}



//uint8_t a[] ={0x01,0x00,0x00,0x00,0x04,0x27,0x23,0x39,0x30,0x31,0x32,0x30,0x30,0x36,0x30,0x30,0x30,0x30,0x31,0x38,0x6c,0xde,0x5a,0x00,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x09,0xc4,0x00,0x00,0x00,0x91,0x00,0x93,0x00,0x9a,0x00,0x9e,0x00,0xa3,0x00,0xa7,0x00,0xab,0x00,0xaf,0x00,0xb4,0x00,0xb8,0x00,0xbc,0x00,0xc0,0x00,0xc5,0x00,0xc9,0x00,0xcd,0x00,0xd2,0x00,0xd6,0x00,0xda,0x00,0xde,0x00,0xe3,0x00,0xe8,0x00,0xed,0x00,0xf2,0x00,0xf7,0x00,0xfd,0x01,0x05,0x01,0x0d,0x01,0x19,0x01,0x29,0x01,0x3b,0x01,0x52,0x01,0x6c,0x01,0x8b,0x01,0xaa,0x01,0xc7,0x01,0xe6,0x02,0x09,0x00,0xb5,0x00,0x22,0x00,0x1c,0x00,0x20,0x00,0x4b,0x00,0x18,0x00,0x0c,0x00,0x0a,0x00,0x08,0x00,0x06,0x00,0x05,0x00,0x05,0x00,0x06,0x00,0x0a,0x00,0x1d,0x00,0x55,0x00,0x96,0x00,0x5a,0x00,0x05,0x00,0x26,0x01,0x3e,0x01,0x8b,0x01,0xad,0x01,0xc5,0x01,0xd8,0x01,0xe8,0x01,0xf5,0x02,0x01,0x02,0x0d,0x02,0x19,0x02,0x26,0x02,0x32,0x02,0x3e,0x02,0x49,0x02,0x52,0x02,0x5d,0x02,0x65,0x02,0x6c,0x02,0x74,0x02,0x7b,0x02,0x82,0x02,0x8a,0x02,0x91,0x02,0x9b,0x02,0xa4,0x02,0xac,0x02,0xb1,0x02,0xb7,0x02,0xbb,0x02,0xbd,0x02,0xbf,0x02,0xc1,0x02,0xc5,0x02,0xc7,0x02,0xca,0x02,0xcd,0x02,0xd0,0x02,0xd3,0x02,0xd5,0x02,0xd8,0x02,0xda,0x02,0xdd,0x02,0xdf,0x02,0xe2,0x02,0xe3,0x02,0xe6,0x02,0xe8,0x02,0xea,0x02,0xec,0x02,0xec,0x02,0xef,0x02,0xf0,0x02,0xf2,0x02,0xf4,0x02,0xf6,0x02,0xf7,0x02,0xf9,0x02,0xfa,0x02,0xfd,0x02,0xfe,0x02,0xfe,0x02,0xff,0x03,0x01,0x03,0x02,0x03,0x04,0x03,0x04,0x03,0x06,0x03,0x06,0x03,0x08,0x03,0x09,0x03,0x0c,0x03,0x0d,0x03,0x0e,0x03,0x0f,0x03,0x10,0x03,0x11,0x03,0x13,0x03,0x13,0x03,0x14,0x03,0x16,0x03,0x17,0x03,0x17,0x03,0x19,0x03,0x1a,0x03,0x1a,0x03,0x1b,0x03,0x1c,0x03,0x1e,0x03,0x1d,0x03,0x1d,0x03,0x1f,0x03,0x20,0x03,0x20,0x03,0x21,0x03,0x21,0x03,0x22,0x03,0x23,0x03,0x25,0x03,0x25,0x03,0x26,0x03,0x25,0x03,0x26,0x03,0x27,0x03,0x28,0x03,0x29,0x03,0x2b,0x03,0x2b,0x03,0x2b,0x03,0x2b,0x03,0x2c,0x03,0x2c,0x03,0x2d,0x03,0x2d,0x03,0x2d,0x03,0x2e,0x03,0x30,0x03,0x30,0x03,0x30,0x03,0x31,0x03,0x32,0x03,0x31,0x03,0x32,0x03,0x32,0x03,0x33,0x03,0x34,0x03,0x34,0x03,0x34,0x03,0x34,0x03,0x35,0x03,0x35,0x03,0x35,0x03,0x37,0x03,0x37,0x03,0x38,0x03,0x39,0x03,0x38,0x03,0x38,0x03,0x38,0x03,0x39,0x03,0x3a,0x03,0x3a,0x03,0x3a,0x03,0x3b,0x03,0x39,0x03,0x3a,0x03,0x3b,0x03,0x3b,0x03,0x3c,0x03,0x3c,0x03,0x3d,0x03,0x3c,0x03,0x3c,0x03,0x3d,0x03,0x3c,0x03,0x3d,0x03,0x3f,0x03,0x3e,0x03,0x3e,0x03,0x3f,0x03,0x40,0x03,0x3f,0x03,0x3f,0x03,0x40,0x03,0x40,0x03,0x41,0x03,0x40,0x03,0x41,0x03,0x43,0x03,0x42,0x03,0x42,0x03,0x42,0x03,0x42,0x03,0x43,0x03,0x44,0x03,0x43,0x03,0x42,0x03,0x44,0x03,0x43,0x03,0x44,0x03,0x45,0x03,0x44,0x03,0x44,0x03,0x43,0x03,0x45,0x03,0x47,0x03,0x45,0x03,0x45,0x03,0x44,0x03,0x45,0x03,0x45,0x03,0x45,0x03,0x46,0x03,0x46,0x03,0x47,0x03,0x46,0x03,0x47,0x03,0x47,0x03,0x46,0x03,0x47,0x03,0x46,0x03,0x47,0x03,0x48,0x03,0x45,0x03,0x47,0x03,0x48,0x03,0x48,0x03,0x47,0x03,0x48,0x03,0x47,0x03,0x49,0x03,0x49,0x03,0x48,0x03,0x48,0x03,0x48,0x03,0x48,0x03,0x49,0x03,0x4a,0x03,0x49,0x03,0x48,0x03,0x48,0x03,0x4a,0x03,0x49,0x03,0x4a,0x03,0x49,0x03,0x4a,0x03,0x47,0x03,0x4a,0x03,0x4a,0x03,0x4b,0x03,0x4a,0x03,0x49,0x03,0x49,0x03,0x4a,0x03,0x4a,0x03,0x4a,0x03,0x4b,0x03,0x49,0x03,0x49,0x03,0x4a,0x03,0x4b,0x03,0x4c,0x03,0x4b,0x03,0x4b,0x03,0x4b,0x03,0x4b,0x03,0x4a,0x03,0x4b,0x03,0x4b,0x03,0x4a,0x03,0x4b,0x03,0x4a,0x03,0x4b,0x03,0x4d,0x03,0x4c,0x03,0x4c,0x03,0x4a,0x03,0x4b,0x03,0x4b,0x03,0x4b,0x03,0x4c,0x03,0x4d,0x03,0x4c,0x03,0x4d,0x03,0x4b,0x03,0x4b,0x03,0x4d,0x03,0x4c,0x03,0x4b,0x03,0x4c,0x03,0x4c,0x03,0x4b,0x03,0x4c,0x03,0x4c,0x03,0x4a,0x03,0x4c,0x03,0x4c,0x03,0x4b,0x03,0x4d,0x03,0x4d,0x03,0x4c,0x03,0x4d,0x03,0x4d,0x03,0x4b,0x03,0x4b,0x03,0x4d,0x03,0x4c,0x03,0x4c,0x03,0x4d,0x03,0x4b,0x03,0x4c,0x03,0x4d,0x03,0x4d,0x03,0x4e,0x03,0x4d,0x03,0x4c,0x03,0x4d,0x03,0x4d,0x03,0x4d,0x03,0x4e,0x03,0x4f,0x03,0x4d,0x03,0x4d,0x03,0x4e,0x03,0x4c,0x03,0x4f,0x03,0x4d,0x03,0x4e,0x03,0x4d,0x03,0x4d,0x03,0x4e,0x03,0x4d,0x03,0x4c,0x03,0x4d,0x03,0x4f,0x03,0x4d,0x03,0x4e,0x03,0x4c,0x03,0x4e,0x03,0x4d,0x03,0x4c,0x03,0x4c,0x03,0x4d,0x03,0x4d,0x03,0x4e,0x03,0x4f,0x03,0x4e,0x03,0x4d,0x03,0x4e,0x03,0x4e,0x03,0x4d,0x03,0x4e,0x03,0x4d,0x03,0x4d,0x03,0x4f,0x03,0x4e,0x03,0x4e,0x03,0x4e,0x03,0x4d,0x03,0x4f,0x03,0x50,0x03,0x4f,0x03,0x4f,0x03,0x4f,0x03,0x4c,0x03,0x4d,0x03,0x4e,0x03,0x4d,0x03,0x4d,0x03,0x4d,0x03,0x4d,0x03,0x4d,0x03,0x4d,0x03,0x4d,0x03,0x4e,0x03,0x4d,0x03,0x4d,0x03,0x4c,0x03,0x4d,0x03,0x4d,0x03,0x4e,0x03,0x4d,0x03,0x4f,0x03,0x4d,0x03,0x4e,0x03,0x4e,0x03,0x4f,0x03,0x4f,0x03,0x4f,0x03,0x4f,0x03,0x4f,0x03,0x4e,0x03,0x4e,0x03,0x4d,0x03,0x4f,0x03,0x4c,0x03,0x4e,0x03,0x4f,0x03,0x4f,0x03,0x4d,0x03,0x4f,0x03,0x4e,0x03,0x4e,0x03,0x4c,0x03,0x4e,0x03,0x4f,0x03,0x4f,0x03,0x4d,0x03,0x4e,0x03,0x4c,0x03,0x4b,0x03,0x4e,0x03,0x4e,0x03,0x4d,0x03,0x4e,0x03,0x4e,0x03,0x4e,0x03,0x4d,0x03,0x4e,0x03,0x4c,0x03,0x4d,0x03,0x4e,0x03,0x4e,0x03,0x4e,0x03,0x4d,0x03,0x4d,0x03,0x4f,0x03,0x4e,0x03,0x4f,0x03,0x4e,0x03,0x4f,0x03,0x4d,0x03,0x4e,0x03,0x4e,0x03,0x4e,0x03,0x4f,0x03,0x4e,0x03,0x4d,0x03,0x4e,0x03,0x4d,0x03,0x4d,0x03,0x4e,0x03,0x4f,0x03,0x4d,0x03,0x4f,0x03,0x4e,0x03,0x4e,0x03,0x4f,0x03,0x51,0x03,0x4f,0x03,0x4e,0x03,0x4e,0x03,0x4e,0x03,0x4f,0x03,0x4f,0x03,0x4b,0x03,0x4e,0x03,0x4f,0x03,0x4f,0x03,0x4f,0x03,0x4e,0x03,0x4f,0x03,0x4e,0x03,0x4f,0x03,0x4d,0x03,0x4f,0x03,0x4f,0x03,0x4e,0x03,0x4e,0x03,0x4f,0x03,0x4e,0x03,0x4d,0x03,0x4d,0x03,0x4f,0x03,0x4d,0x03,0x51};

//	uint16_t t;
//void cc()
//{
//	t = MB_CRC16(a,1070);
//}

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




