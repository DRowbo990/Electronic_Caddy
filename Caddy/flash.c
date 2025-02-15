 #include "flash.h"
 #define CLUB_ADDRESS 0x080FF800
 #define SETTINGS_ADDRESS 0x08080000
 #define POSITION_ADDRESS 0x08080800
 
 /** @brief Wait until Last Operation has Ended
 * This loops indefinitely until an operation (write or erase) has completed
 * by testing the busy flag.
 */
void flash_wait_for_last_operation(void)
{
        while ((FLASH->SR & FLASH_SR_BSY) == FLASH_SR_BSY);
}
 
/** @brief Clear the Programming Sequence Error Flag
 * This flag is set when incorrect programming configuration has been made.
 */
void flash_clear_pgserr_flag(void)
{
        FLASH->SR |= FLASH_SR_PGSERR;
}
 
/** Clear programming size error flag */
void flash_clear_size_flag(void)
{
        FLASH->SR |= FLASH_SR_SIZERR;
}
 
/** @brief Clear the Programming Alignment Error Flag
 */
void flash_clear_pgaerr_flag(void)
{
        FLASH->SR |= FLASH_SR_PGAERR;
}
 
/** @brief Clear the Write Protect Error Flag
 */
void flash_clear_wrperr_flag(void)
{
        FLASH->SR |= FLASH_SR_WRPERR;
}
 
/** @brief Clear the Programming Error Status Flag
 */
void flash_clear_progerr_flag(void)
{
        FLASH->SR |= FLASH_SR_PROGERR;
}

void flash_clear_eop_flag(void)
{
        FLASH->SR |= FLASH_SR_EOP;
}
 
/** @brief Clear All Status Flags
 * Program error, end of operation, write protect error, busy.
 */
void flash_clear_status_flags(void)
{
        flash_clear_pgserr_flag();
        flash_clear_size_flag();
        flash_clear_pgaerr_flag();
        flash_clear_wrperr_flag();
        flash_clear_progerr_flag();
        flash_clear_eop_flag();
}
 
/** @brief Lock the Option Byte Access
 * This disables write access to the option bytes. It is locked by default on
 * reset.
 */
void flash_lock_option_bytes(void){
        FLASH->CR |= FLASH_CR_OPTLOCK;
}
 
/** @brief Program a 64 bit word to FLASH
 *
 * This performs all operations necessary to program a 64 bit word to FLASH memory.
 * The program error flag should be checked separately for the event that memory
 * was not properly erased.
 *
 * @param[in] address Starting address in Flash.
 * @param[in] data Double word to write
 */
void flash_program_double_word(uint32_t address, uint64_t data)
{
	unlock_flash();
  /* Ensure that all flash operations are complete. */
  flash_wait_for_last_operation();
 
  /* Enable writes to flash. */
  FLASH->CR |= FLASH_CR_PG;
 
  /* Program the each word separately. */
  MMIO32(address) = (uint32_t)data;
  MMIO32(address+4) = (uint32_t)(data >> 32);
 
  /* Wait for the write to complete. */
  flash_wait_for_last_operation();
				
	/* Wait for the EOP flag to be set then clear it */
	while((FLASH->SR & FLASH_SR_EOP) == FLASH_SR_EOP);
	FLASH->SR &= ~FLASH_SR_EOP;
	flash_clear_status_flags();
  /* Disable writes to flash. */
  FLASH->CR &= ~FLASH_CR_PG;
	lock_flash();
}

void unlock_flash(void)
{
	// Write sequence in flash register to unlock
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xCDEF89AB;
}

void lock_flash(void)
{
	FLASH->CR |= FLASH_CR_LOCK;
}

void flash_erase_page(uint8_t bank, uint8_t page)
{
	unlock_flash();
	flash_wait_for_last_operation();
	flash_clear_status_flags();
	FLASH->CR |= FLASH_CR_PER;
	FLASH->CR |= page << 3;
	if(bank == 1)
	{
		FLASH->CR &= ~FLASH_CR_BKER;
	}
	else
	{
		FLASH->CR |= FLASH_CR_BKER;
	}
	FLASH->CR |= FLASH_CR_STRT;
	flash_wait_for_last_operation();
	FLASH->CR &= ~FLASH_CR_PER;
	lock_flash();
}

void readClubs(uint16_t* clubs)
{
	uint64_t data = 0;
	uint16_t copy[12];
	for(int i = 0; i < 3; i++)
	{
		data = (*(volatile uint64_t *)(CLUB_ADDRESS+8*i));
		copy[i*4] = (uint16_t)data;
		data = data >> 16;
		copy[i*4+1] = (uint16_t)data;
		data = data >> 16;
		copy[i*4+2] = (uint16_t)data;
		data = data >> 16;
		copy[i*4+3] = (uint16_t)data;
	}
	for(int i = 0; i < 12; i++)
	{
		clubs[i] = copy[i];
	}
}

void writeClubs(uint16_t* clubs)
{
	flash_erase_page(2, 255);
	uint64_t data = 0;
	uint8_t offset = 0;
	for(int i = 0; i < 12; i = i+4)
	{
		data = clubs[i];
		data |= ((uint64_t)clubs[i+1] << 16);
		data |= ((uint64_t)clubs[i+2] << 32);
		data |= ((uint64_t)clubs[i+3] << 48);
		flash_program_double_word(CLUB_ADDRESS+offset, data);
		offset += 8;
	}
}

void readSettings(uint16_t* settings)
{
	uint64_t data = 0;
	uint16_t copy[2];
	data = (*(volatile uint64_t *)(SETTINGS_ADDRESS));
	copy[0] = (uint16_t)data;
	data = data >> 16;
	copy[1] = (uint16_t)data;
	for(int i = 0; i < 2; i++)
	{
		settings[i] = copy[i];
	}
}

void writeSettings(uint16_t* settings)
{
	flash_erase_page(2, 0);
	uint64_t data = 0;
	data = settings[0];
	data |= ((uint64_t)settings[1] << 16);
	flash_program_double_word(SETTINGS_ADDRESS, data);
}

uint16_t readPosition(void)
{
	uint64_t data = 0;
	uint16_t copy = 0;
	data = (*(volatile uint64_t *)(POSITION_ADDRESS));
	copy = (uint16_t)data;
	if(copy > 12 || copy == 0)
	{
		copy = 1;
	}
	return copy;
}

void writePosition(uint16_t position)
{
	flash_erase_page(2, 1);
	uint64_t data = 0;
	data = position;
	flash_program_double_word(POSITION_ADDRESS, data);
}
