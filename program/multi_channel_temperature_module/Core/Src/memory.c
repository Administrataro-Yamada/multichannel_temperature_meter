/*
 * global_typedef.c
 *
 *  Created on: 2021/10/26
 *      Author: staff
 */

#include <memory.h>
#include "main.h"

#define MEMORY_PAGE_STATUS 15

static Memory_TypeDef memory  = {0};

static const uint32_t memory_address_status = 0x08000000 + MEMORY_PAGE_STATUS*1024;



static uint32_t _Flash_Erase(){
	FLASH_EraseInitTypeDef erase;
	erase.TypeErase = FLASH_TYPEERASE_PAGES;
	erase.Page = memory_address_status;//TODO
	erase.NbPages = 1;

	uint32_t pageError = 0;
	HAL_FLASHEx_Erase(&erase, &pageError);

	return pageError;
}

//size : 2word = 4byte
static void Memory_BackUp(){
	if(memory.sram.backup_request == 1){
		memory.sram.backup_request = 0;
		HAL_FLASH_Unlock();
		_Flash_Erase();
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, memory_address_status, memory.flash.baudrate<<8 | memory.flash.slave_address);
		//HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, memory_address_status + 2, _global_struct.hex_offset);
		HAL_FLASH_Lock();
	}
}

static void Memory_Load(){
	if(memory.sram.load_request == 1){
		memory.sram.load_request = 0;
		uint16_t *addr = (uint16_t *)memory_address_status;
		memory.flash.slave_address = *addr & 0xff;
		memory.flash.baudrate = ((*addr)>>8) & 0xff;

		//_global_struct.hex_offset = *(addr+1);
	}
}

uint8_t Memory_Get_Slave_Address(){
	return memory.flash.slave_address;
}

uint8_t Memory_Get_Baudrate(){
	return memory.flash.baudrate;
}
void Memory_Set_Backup_Request(){
	memory.sram.backup_request = 1;
}

void Memory_Cyclic(){
	Memory_BackUp();
	Memory_Load();
}


void Global_Init(){
	memory.sram.tim_on_going = 0;
	memory.sram.load_request = 1;
	memory.sram.tx_on_going = 0;
//	_global_struct.hex_output = 0x7fff;
//	_global_struct.io_atm = 1;


}
