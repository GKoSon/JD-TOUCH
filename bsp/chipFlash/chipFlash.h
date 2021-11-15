#ifndef _CHIP_FLASH_H_
#define _CHIP_FLASH_H_


/* Error code */
enum 
{
  FLASHIF_OK = 0,
  FLASHIF_ERASEKO,
  FLASHIF_WRITINGCTRL_ERROR,
  FLASHIF_WRITING_ERROR,
  FLASHIF_PROTECTION_ERRROR
};

void chip_flash_init( void );
uint32_t chip_flash_read( uint32_t addr , uint8_t* data , uint16_t len );       
uint32_t chip_flash_write( uint32_t addr , uint8_t* data , uint16_t len);
uint32_t chip_flash_earse( uint32_t addr );
uint8_t chip_flash_get_lock( void );
void chip_flash_release_lock( void );

#endif

