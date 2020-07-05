/**
  ******************************************************************************
  * @file           : boot.c
  * @brief          : 
  ******************************************************************************
  * @attention			: 检查固件区固件是否有效和正确，
  *
  *
  ******************************************************************************
  */



#include "main.h"

#pragma pack(1)

typedef struct _firmware_head
{
	uint32_t head;
	uint32_t len;
	uint32_t crc;
	uint8_t data[1];
}struct_firmware_head; //flash中存储的固件头


typedef struct _firmware_protocol_head
{
	uint32_t len;
	uint32_t now_index;
	uint32_t crc;
}struct_firmware_protocol_head; //flash中存储的固件头






#pragma pack()


#define RUN_FIRMWARE_ADDR (0X08000000+16*1024)           		//APP运行地址

#define STORE_FIRMWARE_ADDR (0X08000000+16*1024+100*1024)   	//固件存放地址



struct_firmware_head *p_firmware_head = (struct_firmware_head *)STORE_FIRMWARE_ADDR;

extern CRC_HandleTypeDef hcrc;

struct_firmware_protocol_head firmware_protocol_head;









//擦除固件存储区
HAL_StatusTypeDef erase_firmware_store(void)
{
	FLASH_EraseInitTypeDef EraseInit;
	HAL_StatusTypeDef HAL_Status;
	uint32_t PageError;
	
	EraseInit.PageAddress = STORE_FIRMWARE_ADDR;
	EraseInit.Banks = FLASH_BANK_1;
	EraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInit.NbPages = 50;
	
	HAL_FLASH_Unlock();
	HAL_Status = HAL_FLASHEx_Erase(&EraseInit, &PageError);	
	HAL_FLASH_Lock();
	
	return HAL_Status;
	
}

//写入flash数据 约定包长度是4的倍数 最后一包可忽略 
//return:0  写入成功 
//       -1 擦除失败    终止程序下载
//       -2 擦除失败    终止程序下载

int32_t write_firmware(uint32_t addr,void *p_data,uint16_t len)
{
	uint16_t i,word_len,delay;
	uint32_t Address = addr;
	uint32_t *pdata = (uint32_t *)p_data;
	HAL_StatusTypeDef HAL_Status;
	
	if(addr == STORE_FIRMWARE_ADDR+12)  //第一包  先执行擦除
	{
		if(HAL_OK != erase_firmware_store())
		{
			delay = 10000;
			while(delay--);
			if(HAL_OK != erase_firmware_store())
			{
				return -1;
			}
		}
	}

	word_len = len/4;
	if(len%4 != 0)
		word_len++;
	
	HAL_FLASH_Unlock();
	
	for(i = 0;i<word_len;i++)
	{
		HAL_Status =  HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, pdata[i]);
		if(*((uint32_t *)Address) != pdata[i])//写失败 再尝试写2次
		{
			HAL_Status =  HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, pdata[i]);
			if(*((uint32_t *)Address) != pdata[i])//写失败 再尝试写3次
			{
				HAL_Status =  HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, pdata[i]);
				if(*((uint32_t *)Address) != pdata[i])//写失败 复位
				{
					return -2;
				}						
			}					
		}
		Address +=4;
	}
	//write good
	HAL_FLASH_Lock();		
	return 0;

}



int32_t crc_firmware(uint32_t len)
{
	uint32_t crc_value;
	
	if(len>1024*100)
		return -1;
	
	crc_value = HAL_CRC_Calculate(&hcrc,(uint32_t *)(p_firmware_head->data),(p_firmware_head->len/4+1));
	if(firmware_protocol_head.crc == crc_value)	
	{
	
	}
	return crc_value;

}




