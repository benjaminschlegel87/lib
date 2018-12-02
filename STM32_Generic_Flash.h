#ifndef _FSM_STM32_Generic_Flash_H
#define _FSM_STM32_Generic_Flash_H

#include <stdbool.h>
#include <string.h>
#include "stm32f3xx_hal.h"



/*
Hier wird definiert für welchen Controller der Bootloader verwendet wird. Die HAL Flash Funktionen sind für alle identisch nur die unterstützten Formate 
unterscheiden sich. Daher wird hier FLASH_TYPEPROGRAMM und der ALIGN_FACTOR definiert. Auch wird der 8Bit Daten pointer "data" als Übergabe Wert der Write_Flash Funktion 
entsprechend gecastet

*/
#ifdef STM32L4XX
#define FLASH_TYPE FLASH_TYPEPROGRAM_DOUBLEWORD
#define ALIGN_FACTOR 8
#define ALIGNED_POINTER (uint64_t*)data
#elif STM32F3XX 
#define FLASH_TYPE FLASH_TYPEPROGRAM_WORD
#define ALIGN_FACTOR 4
#define ALIGNED_POINTER (uint32_t*)data
#elif STM32L0XX
#define FLASH_TYPE FLASH_TYPEPROGRAM_WORD
#define ALIGN_FACTOR 4
#define ALIGNED_POINTER (uint32_t*)data
#else
#error "Keinen Controller für Flash Konfiguration ausgewählt"
#endif


enum eReturnTypes
{
	FSM_STM32_FLASH_SUCCESS = 1,
	FSM_STM32_FLASH_ALIGNMENT_ERROR,
	FSM_STM32_FLASH_ERASE_FAILED
	
};




uint32_t Delete_Flash_Pages(uint32_t num_of_pages,uint32_t adress);

uint32_t Write_Flash(uint8_t* data,uint32_t length,uint32_t adress);

#endif

