
#ifndef _FSM_STM32_Bootloader_H
#define _FSM_STM32_Bootloader_H

#include <stdbool.h>
#include <string.h>
#include "stm32f3xx_hal.h"



#define BASE_ADRESS				 	0x8000000
#define BOOTLOADER_SIZE 		 	0x6000
#define BOOTLOADER_SETTINGS_SIZE 	0x800
#define BOOTLOADER_SETTINGS_ADRESS  ((BASE_ADRESS+BOOTLOADER_SIZE)-BOOTLOADER_SETTINGS_SIZE)


#define PAGE_SIZE 0x800





typedef enum
{
	FSM_STM32_SUCCESS = 1,
	VALID_APP_NO_BOOTLOADER_REQUEST,		// 2
	STAY_IN_BOOTLOADER,						// 3
	UPDATE_INTERRUPTED_CAN_CONTINUE,		// 4	
	FRESH_UPDATE,							// 5
	OLD_APP_VERSION,						// 6
	NO_VALID_INIT_PACKET,					// 7
	DATA_NOT_PAGE_ALIGNED,					// 8
	PAGE_WRITTEN,							// 9
	UPDATE_COMPLETE,						// 10
	PAGE_NOT_COMPLETE,						// 11
	FINAL_CRC_ERROR,						// 12
	TOO_MUCH_DATA_WRITTEN,					// 13
	PAGE_CRC_ERROR,							// 14
	NO_VALID_APP,
	VALID_APP_CRC_ERROR,
	
}eBootloaderResult;

typedef enum
{
	INIT_BOOTLOADER = 0,	/*!< Initializes Bootloader and Bootload Settings. Returns no Data just Result */
	SET_INIT_PACKET,		/*!< Sets a new Init Package */
	WRITE_DATA,				/*!< Writes data to page buffer. If page buffer == page size -> writes page to flash */
	JUMP,
	READ_SETTINGS,
}eBootloaderTaks;





typedef struct
{
	uint32_t app_size;
	uint32_t app_version;
	uint32_t app_crc;
	uint32_t start_adress;
}sBootloaderInitPacket;


// needs to be 64bit aligned for L4
typedef struct
{
	uint32_t settings_crc;
	uint32_t last_valid_app_version;	
	uint32_t last_valid_app_size;
	uint32_t last_valid_app_crc;
	uint32_t last_valid_start_adress;
	uint32_t is_valid_app;
	uint32_t is_jump_from_app;
	uint32_t page_progress;
	sBootloaderInitPacket current_init_packet;  
}sBootloaderSettings;


typedef struct
{
	eBootloaderTaks task;
	uint8_t* data;
	size_t length;
	sBootloaderSettings settings;
	eBootloaderResult result;
	size_t pages;
	size_t bytes;
	
}sBootloaderEvt;










/* EXPORTED FUNCTIONS */

void FSM_STM32_Bootloader_Event(sBootloaderEvt* evt);







#endif
