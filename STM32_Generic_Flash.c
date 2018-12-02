#include "FSM_STM32_Generic_Flash.h"







/*
Writes Array of Bytes to Flash
Length must be aligned to Word Type of the HAL_Flash_Program Function for used controller

*/
uint32_t Write_Flash(uint8_t* data,uint32_t length,uint32_t adress)
{
	
	
	// Check for alignment
	
	if(length%ALIGN_FACTOR != 0)
	{
	
		return FSM_STM32_FLASH_ALIGNMENT_ERROR; // Segmentation Error
	}
	
	HAL_FLASH_Unlock();
	
	for(uint32_t n=0;n<(length/ALIGN_FACTOR);n++)
	{
		HAL_FLASH_Program(FLASH_TYPE,adress+(ALIGN_FACTOR*n),*(ALIGNED_POINTER+n));
	}
	
	HAL_FLASH_Lock();
	
	return FSM_STM32_FLASH_SUCCESS;
	
	
	
	
	
	
}


uint32_t Delete_Flash_Pages(uint32_t num_of_pages,uint32_t adress)
{
	uint32_t error = 0;
	FLASH_EraseInitTypeDef init;
	memset(&init, 0, sizeof(init));
	init.TypeErase = FLASH_TYPEERASE_PAGES;
	init.NbPages = num_of_pages;
	init.PageAddress = adress;
	
	HAL_FLASH_Unlock();
	
	HAL_FLASHEx_Erase(&init,&error);
	
	HAL_FLASH_Lock();
	
	if(error != 0xFFFFFFFF)
	{
		return FSM_STM32_FLASH_ERASE_FAILED;
	}
	
	return FSM_STM32_FLASH_SUCCESS; // to do  error handling
}
