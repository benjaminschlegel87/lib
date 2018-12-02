#include "FSM_STM32_Bootloader.h"
#include "FSM_STM32_Generic_Flash.h"



static sBootloaderSettings Boot_Settings;


extern CRC_HandleTypeDef hcrc;

// local typedef




static void write_Bootloader_Settings(void)
{
	Boot_Settings.settings_crc = HAL_CRC_Calculate(&hcrc,((uint32_t*)&Boot_Settings)+1,(sizeof(Boot_Settings)/4)-4);
	Delete_Flash_Pages(1,BOOTLOADER_SETTINGS_ADRESS);
	Write_Flash((uint8_t*)&Boot_Settings,sizeof(Boot_Settings),BOOTLOADER_SETTINGS_ADRESS);	
	
	
	
}





static bool FSM_STM32_Bootloader_Settings_Init(void)
{

	
	uint32_t* settings_pointer = (uint32_t*)BOOTLOADER_SETTINGS_ADRESS;
	memcpy(&Boot_Settings, settings_pointer, sizeof(Boot_Settings));

	
	if(Boot_Settings.settings_crc != 0xFFFFFFFF)
	{

		if(Boot_Settings.settings_crc == HAL_CRC_Calculate(&hcrc,((uint32_t*)&Boot_Settings)+1,(sizeof(Boot_Settings)/4)-4))
		{

			return true;
		}
		
	}
	


	memset(&Boot_Settings, 0, sizeof(Boot_Settings));
	

	write_Bootloader_Settings();
	
	return false;
}






void Jump_to_Bootloader(void)
{
	FSM_STM32_Bootloader_Settings_Init();
	Boot_Settings.is_jump_from_app = 1;
	write_Bootloader_Settings();
	HAL_Delay(500);
	NVIC_SystemReset();
	
}
