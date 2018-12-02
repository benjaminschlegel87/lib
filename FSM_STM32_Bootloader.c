#include "FSM_STM32_Bootloader.h"
#include "FSM_STM32_Generic_Flash.h"

#define NUM_OF_PAGES_NEW (Boot_Settings.current_init_packet.app_size/PAGE_SIZE)
#define NUM_OF_PAGES_OLD (Boot_Settings.last_valid_app_size/PAGE_SIZE)
#define BYTES_OF_LAST_PAGE (Boot_Settings.current_init_packet.app_size - (NUM_OF_PAGES_NEW*PAGE_SIZE))

static sBootloaderSettings Boot_Settings;
static uint8_t page_buffer[PAGE_SIZE];
static bool update_enabled = false;
static uint32_t counter = 0;


/* JUMP */
typedef  void (*pFunction)(void);
uint32_t appStack;
pFunction appEntry;


extern CRC_HandleTypeDef hcrc;

// local typedef
typedef struct
{
	eBootloaderResult result_code;
	uint32_t pages;
	uint32_t bytes;
	
}sBootloaderReturnData;

static void Jump_to_App(void)
{
		//Jump to address
		/* Get the application stack pointer */
		appStack = (uint32_t) * ((__IO uint32_t*)Boot_Settings.last_valid_start_adress);
		/* Get the application entry point */
		appEntry = (pFunction) * (__IO uint32_t*) (Boot_Settings.last_valid_start_adress + 4);

		/* Reconfigure vector table offset */
		SCB->VTOR = Boot_Settings.last_valid_start_adress;

		__set_MSP(appStack);

		appEntry();
}




static void write_Bootloader_Settings(void)
{
	Boot_Settings.settings_crc = HAL_CRC_Calculate(&hcrc,((uint32_t*)&Boot_Settings)+1,(sizeof(Boot_Settings)/4)-4);
	Delete_Flash_Pages(1,BOOTLOADER_SETTINGS_ADRESS);
	Write_Flash((uint8_t*)&Boot_Settings,sizeof(Boot_Settings),BOOTLOADER_SETTINGS_ADRESS);	
	
	
	
}


static eBootloaderResult FSM_Bootloader_Write_Page(void)
{
	eBootloaderResult result;
	if(Boot_Settings.is_valid_app == 1)
	{
		Boot_Settings.is_valid_app = 0;
		write_Bootloader_Settings();
	}

	Delete_Flash_Pages(1,Boot_Settings.current_init_packet.start_adress+((Boot_Settings.page_progress)*PAGE_SIZE));
	Write_Flash(page_buffer,PAGE_SIZE,Boot_Settings.current_init_packet.start_adress+((Boot_Settings.page_progress)*PAGE_SIZE));
	
	if( (HAL_CRC_Calculate(&hcrc,(uint32_t*)page_buffer,PAGE_SIZE/4)) != (HAL_CRC_Calculate(&hcrc,(uint32_t*)(Boot_Settings.current_init_packet.start_adress+((Boot_Settings.page_progress)*PAGE_SIZE)),PAGE_SIZE/4)) ) 
	{
		Delete_Flash_Pages(1,Boot_Settings.current_init_packet.start_adress+((Boot_Settings.page_progress)*PAGE_SIZE));
		result = PAGE_CRC_ERROR;
	}
	else
	{
	
		Boot_Settings.page_progress++;
		write_Bootloader_Settings();
		result = PAGE_WRITTEN;
	}
	memset(page_buffer, 0xFF, sizeof(page_buffer));	
	return result;		
	
		
	
	

}

static void FSM_Bootloader_Reset_Page(void)
{
	counter = 0;
	
	
}


static sBootloaderReturnData FSM_Bootloader_Fill_Page(uint8_t* data,uint32_t length)
{
	
	sBootloaderReturnData result;
	memset(&result, 0, sizeof(sBootloaderReturnData));
	
	if(update_enabled == 0)
	{
		result.result_code = NO_VALID_INIT_PACKET;
		return result;
	}

	
	if( (counter+length) > (PAGE_SIZE) )
	{
		//wrong alignment page is full cant fit all data
		counter = 0;
		result.result_code = DATA_NOT_PAGE_ALIGNED;

		return result;
	}
	
	for(uint32_t n=0;n<length;n++)
	{
		page_buffer[counter] = data[n];
		counter++;
	}
	
	result.bytes = counter;
	result.pages = Boot_Settings.page_progress;
	
	if( counter == (PAGE_SIZE))
	{
		
		result.result_code = FSM_Bootloader_Write_Page();
		counter = 0;
		
		return result; // page complete
	}
	
	if( (Boot_Settings.page_progress == NUM_OF_PAGES_NEW) && (counter == BYTES_OF_LAST_PAGE))
	{
		result.result_code = FSM_Bootloader_Write_Page();
		
		if(result.result_code == PAGE_CRC_ERROR)
		{
			counter = 0;
			return result;
		}
		counter = 0;

		uint32_t crc = HAL_CRC_Calculate(&hcrc,(uint32_t*)Boot_Settings.current_init_packet.start_adress,Boot_Settings.current_init_packet.app_size/4);
		/* Crc Check over new App Size */
		if(crc != Boot_Settings.current_init_packet.app_crc)
		{
			Boot_Settings.page_progress = 0;
			memset(&Boot_Settings.current_init_packet, 0, sizeof(sBootloaderInitPacket));
			
			
			write_Bootloader_Settings();
			result.result_code = FINAL_CRC_ERROR;
			update_enabled = false;
			return result;
		}
		
		
		
		
		/* Writing new Settings */
		Boot_Settings.is_valid_app = 1;
		Boot_Settings.last_valid_app_size = Boot_Settings.current_init_packet.app_size;
		Boot_Settings.last_valid_app_version = Boot_Settings.current_init_packet.app_version;
		Boot_Settings.last_valid_app_crc = crc;
		Boot_Settings.last_valid_start_adress = Boot_Settings.current_init_packet.start_adress;
		Boot_Settings.page_progress = 0;
		memset(&Boot_Settings.current_init_packet, 0, sizeof(sBootloaderInitPacket));
		
		write_Bootloader_Settings();
		
		
		result.result_code = UPDATE_COMPLETE;
		update_enabled = false;
		return result;
	}
	
	if( (Boot_Settings.page_progress == NUM_OF_PAGES_NEW) && (counter > BYTES_OF_LAST_PAGE))
	{	
		counter = 0;
		result.result_code = TOO_MUCH_DATA_WRITTEN;
		
		Boot_Settings.page_progress = 0;
		memset(&Boot_Settings.current_init_packet, 0, sizeof(sBootloaderInitPacket));
		write_Bootloader_Settings();
		
		
		
		
		update_enabled = false;
		
		return result;
	}
	result.result_code = PAGE_NOT_COMPLETE;
	return result;
	
	
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


static sBootloaderReturnData FSM_STM32_Bootloader_InitPacket(sBootloaderInitPacket* init)
{
	sBootloaderReturnData result;
	memset(&result, 0, sizeof(sBootloaderReturnData));

	
	if(Boot_Settings.last_valid_app_version <= init->app_version)		
	{

		
		if(Boot_Settings.page_progress > 0)	// Update was interrupted
		{

			FSM_Bootloader_Reset_Page();
			if( (init->app_size == Boot_Settings.current_init_packet.app_size)
				&&(init->app_version == Boot_Settings.current_init_packet.app_version)
				&&(init->app_crc == Boot_Settings.current_init_packet.app_crc)
				&&(init->start_adress == Boot_Settings.current_init_packet.start_adress) )
			{
				result.result_code = UPDATE_INTERRUPTED_CAN_CONTINUE;
				result.pages = Boot_Settings.page_progress;
				

				
				// return update can continue + Page Number
			}
			else	//Update was interrupted but new request isnt equal to old update
			{
				// clear old written pages before starting new update
				if( Boot_Settings.page_progress > ((init->app_size)/PAGE_SIZE)) 
				{
					Delete_Flash_Pages( Boot_Settings.page_progress + 1 ,Boot_Settings.current_init_packet.start_adress );
				}
				
				Boot_Settings.page_progress = 0;
				result.result_code = FRESH_UPDATE;	
			}				

		}
		else	// No interrupted Update
		{
	 

			result.result_code = FRESH_UPDATE;
		}
		
	}
	else
	{
	
		result.result_code = OLD_APP_VERSION;
		return result;
	}
	
	// copy new init_packet to settings
	memcpy(&Boot_Settings.current_init_packet, init, sizeof(sBootloaderInitPacket));


	
	write_Bootloader_Settings();
	// initialize buffer with 0xFF 
	/* Program Data is never 0xFF -> with 0xFF init last page is automatically 0xFF where the program data ends */
	memset(page_buffer, 0xFF, sizeof(page_buffer));


	update_enabled = true;
	
	return result;
}



static eBootloaderResult FSM_STM32_Bootloader_Init(void)		//zu überprüfen
{
	
	
	
	FSM_STM32_Bootloader_Settings_Init();
	
	if( (Boot_Settings.is_valid_app == 1) && !(Boot_Settings.is_jump_from_app == 1)) // no reason for Bootloader
	{
		
		if(Boot_Settings.last_valid_app_crc == HAL_CRC_Calculate(&hcrc,(uint32_t*)Boot_Settings.last_valid_start_adress,Boot_Settings.current_init_packet.app_size/4) )
		{
			return VALID_APP_NO_BOOTLOADER_REQUEST;
		}
		
		return VALID_APP_CRC_ERROR;

		
	}
	else
	{
		if(Boot_Settings.is_jump_from_app == 1)
		{
			Boot_Settings.is_jump_from_app = 0;
			write_Bootloader_Settings();
		}

		return STAY_IN_BOOTLOADER;
	}
}

void FSM_STM32_Bootloader_Event(sBootloaderEvt* evt)
{
	sBootloaderReturnData ret;
	memset(&ret, 0, sizeof(ret));
	
	switch(evt->task)
	{
		
		case INIT_BOOTLOADER:
		{
			evt->result = FSM_STM32_Bootloader_Init();
			
		}break;
		
		case SET_INIT_PACKET:
		{
			
			ret = FSM_STM32_Bootloader_InitPacket(&evt->settings.current_init_packet);
			evt->result = ret.result_code;
			switch(ret.result_code)
			{
				
				case UPDATE_INTERRUPTED_CAN_CONTINUE:
				{
					evt->pages = ret.pages;
					
				}break;
				
				case FRESH_UPDATE:
				{
					
				}break;
				
				case OLD_APP_VERSION:
				{
				}break;
				
				default:
				{
					
				}break;
				
			}
			
		}break;
		
		case WRITE_DATA:
		{
			ret = FSM_Bootloader_Fill_Page(evt->data,evt->length);
			evt->result = ret.result_code;
			evt->pages = ret.pages;
			evt->bytes = ret.bytes;
			
		}break;
		
		case JUMP:
		{
			if(Boot_Settings.is_valid_app)
			{
				Jump_to_App();
			}
			
			evt->result = NO_VALID_APP;
			
		}break;
		
		case READ_SETTINGS:
		{
			evt->settings = Boot_Settings;
		}break;
		
		
	}
	
}


