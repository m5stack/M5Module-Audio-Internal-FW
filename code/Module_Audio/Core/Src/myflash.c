#include "myflash.h"

/**
 * @brief  Get the page number according to the address
 * @param  addr Memory address
 * @retval Returns the page number
 */
static uint32_t GetPage(uint32_t addr)
{
    return (addr - STM32G0xx_FLASH_PAGE0_STARTADDR) / FLASH_PAGE_SIZE;
}

/**
 * @brief  Modify a specific byte in an 8-byte (uint64_t) data
 * @param  data Pointer to the data to be modified
 * @param  byte_index Index of the byte to modify
 * @param  new_value New value to set
 * @retval None
 */
static void set_byte_in_uint64(uint64_t *data, uint8_t byte_index, uint8_t new_value)
{
    *data &= ~((uint64_t)(0xFF) << (byte_index * 8));
    // Then set the new byte value
    *data |= (uint64_t)new_value << (byte_index * 8);
}

/**
 * @brief  Read a byte from flash
 * @param  address Memory address
 * @retval Byte value
 */
uint8_t my_flash_read_byte(uint32_t address)
{
    return *((__IO uint8_t *)(address));
}

/**
 * @brief  Read a half-word (2 bytes) from flash
 * @param  address Memory address
 * @retval Half-word value
 */
uint16_t my_flash_read_half_word(uint32_t address)
{
    return *((__IO uint16_t *)(address));
}

/**
 * @brief  Read a word (4 bytes) from flash
 * @param  address Memory address
 * @retval Word value
 */
uint32_t my_flash_read_word(uint32_t address)
{
    return *((__IO uint32_t *)(address));
}

/**
 * @brief  Read a double-word (8 bytes) from flash
 * @param  address Memory address
 * @retval Double-word value
 */
uint64_t my_flash_read_double_word(uint32_t address)
{
    return *((__IO uint64_t *)(address));
}

/**
 * @brief  Erase flash page
 * @param  address Page start address
 * @retval Whether the operation was successful
 */
bool my_flash_earse_pages(uint32_t page_address)
{
    uint32_t page_error = 0;                     // Set page_error; if an error occurs, this will hold the error address
    FLASH_EraseInitTypeDef my_flash;             // Erase location configuration
    my_flash.TypeErase = FLASH_TYPEERASE_PAGES;  // Page erase mode
    my_flash.Page      = GetPage(page_address);
    my_flash.NbPages   = 1;  // Number of pages to erase
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&my_flash, &page_error);
    HAL_FLASH_Lock();
    if (status == HAL_OK) {
        return true;
    }
    return false;
}

/**
 * @brief  Write a half-word to flash
 * @param  address Memory address
 * @param  data Data to write
 * @retval Whether the operation was successful
 */
bool my_flash_write_half_word(uint32_t address, uint16_t data)
{
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);
    HAL_FLASH_Lock();
    if (status == HAL_OK) {
        return true;
    }
    return false;
}

/**
 * @brief  Write a double-word to flash
 * @param  address Memory address
 * @param  data Data to write
 * @retval Whether the operation was successful
 */
static bool my_flash_write_double_word(uint32_t address, uint64_t data)
{
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);
    HAL_FLASH_Lock();
    if (status == HAL_OK) {
        return true;
    }
    return false;
}

bool set_lin_mic_pc_en_status(uint8_t data)
{
    uint64_t temp = my_flash_read_double_word(STM32G0xx_FLASH_PAGE15_STARTADDR);
    set_byte_in_uint64(&temp, 0, data);
    my_flash_earse_pages(STM32G0xx_FLASH_PAGE15_STARTADDR);
    while (my_flash_write_double_word(STM32G0xx_FLASH_PAGE15_STARTADDR, temp) != true);
    uint8_t dat = get_lin_mic_pc_en_status();
    if (dat == data) {
        return true;
    }
    return false;
}

bool set_hp_mode_status(uint8_t data)
{
    uint64_t temp = my_flash_read_double_word(STM32G0xx_FLASH_PAGE15_STARTADDR);
    set_byte_in_uint64(&temp, 1, data);
    my_flash_earse_pages(STM32G0xx_FLASH_PAGE15_STARTADDR);
    while (my_flash_write_double_word(STM32G0xx_FLASH_PAGE15_STARTADDR, temp) != true);
    uint8_t dat = get_hp_mode_status();
    if (dat == data) {
        return true;
    }
    return false;
}

bool set_rgb_light_status(uint8_t data)
{
    uint64_t temp = my_flash_read_double_word(STM32G0xx_FLASH_PAGE15_STARTADDR);
    set_byte_in_uint64(&temp, 2, data);
    my_flash_earse_pages(STM32G0xx_FLASH_PAGE15_STARTADDR);
    while (my_flash_write_double_word(STM32G0xx_FLASH_PAGE15_STARTADDR, temp) != true);
    uint8_t dat = get_rgb_light_status();
    if (dat == data) {
        return true;
    }
    return false;
}

bool set_i2c_addr(uint8_t data)
{
    uint64_t temp = my_flash_read_double_word(STM32G0xx_FLASH_PAGE15_STARTADDR);
    set_byte_in_uint64(&temp, 3, data);
    my_flash_earse_pages(STM32G0xx_FLASH_PAGE15_STARTADDR);
    while (my_flash_write_double_word(STM32G0xx_FLASH_PAGE15_STARTADDR, temp) != true);
    uint8_t dat = get_i2c_addr();
    if (dat == data) {
        return true;
    }
    return false;
}

uint8_t get_lin_mic_pc_en_status(void)
{
    return *((__IO uint8_t *)(LIN_MIC_PC_EN_STATUS_ADDR));
}

uint8_t get_hp_mode_status(void)
{
    return *((__IO uint8_t *)(HP_MODE_STATUS_ADDR));
}

uint8_t get_rgb_light_status(void)
{
    return *((__IO uint8_t *)(RGB_LIGHT_STATUS_ADDR));
}

uint8_t get_i2c_addr(void)
{
    return *((__IO uint8_t *)(I2C_ADDR));
}

bool write_flash(uint8_t pc_en, uint8_t hp_mode, uint8_t rgb_light)
{
    __disable_irq();
    uint64_t temp = my_flash_read_double_word(STM32G0xx_FLASH_PAGE15_STARTADDR);
    set_byte_in_uint64(&temp, 0, pc_en);
    set_byte_in_uint64(&temp, 1, hp_mode);
    set_byte_in_uint64(&temp, 2, rgb_light);
    my_flash_earse_pages(STM32G0xx_FLASH_PAGE15_STARTADDR);
    while (my_flash_write_double_word(STM32G0xx_FLASH_PAGE15_STARTADDR, temp) != true);
    __enable_irq();
    if (pc_en == get_lin_mic_pc_en_status() && hp_mode == get_hp_mode_status() && rgb_light == get_rgb_light_status()) {
        return true;
    }
    return false;
}
