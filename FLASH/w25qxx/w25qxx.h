#ifndef USER_DRIVERS_W25QXX_H_
#define USER_DRIVERS_W25QXX_H_

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C"{
#endif

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define MEMORY_FLASH_SIZE				0x200000 /* 16Mbit =>2Mbyte */
#define MEMORY_BLOCK_SIZE				0x10000   /*  blocks of 64KBytes */
#define MEMORY_SECTOR_SIZE				0x1000    /* 4kBytes */
#define MEMORY_PAGE_SIZE				0x100     /* 256 bytes */

#define W25Q_WRITE_ENABLE 0x06
#define W25Q_WRITE_DISABLE 0x04

#define W25Q_READ_SR1 0x05
#define W25Q_READ_SR2 0x35
#define W25Q_READ_SR3 0x15
#define W25Q_WRITE_SR1 0x01
#define W25Q_WRITE_SR2 0x31
#define W25Q_WRITE_SR3 0x11

#define W25Q_READ_DATA 0x03
#define W25Q_READ_DATA_4B 0x13
#define W25Q_FAST_READ 0x0B
#define W25Q_FAST_READ_4B 0x0C

#define W25Q_PAGE_PROGRAM 0x02
#define W25Q_PAGE_PROGRAM_4B 0x12

#define W25Q_SECTOR_ERASE 0x20
#define W25Q_SECTOR_ERASE_4B 0x21
#define W25Q_32KB_BLOCK_ERASE 0x52
#define W25Q_64KB_BLOCK_ERASE 0xD8
#define W25Q_64KB_BLOCK_ERASE_4B 0xDC
#define W25Q_CHIP_ERASE 0xC7

#define W25Q_ENABLE_RST 0x66
#define W25Q_RESET 0x99
/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                              ENUMS
==================================================================================================*/

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct{
	void (*csHIGH)(void);
	void (*csLOW)(void);
	void (*spi_read)(uint8_t *pdata,uint16_t len);
	void (*spi_write)(uint8_t *pdata,uint16_t len);
	void (*delay_ms)(uint32_t ms);                                                                  /**< point to a delay_ms function address */
	void (*debug_print)(const char *const fmt, ...);                                                /**< point to a debug_print function address */
}w25qxxIF_t;

typedef struct{
	w25qxxIF_t   *meIF;
    uint8_t tempBytes[4];
}w25qxx_handle_t;
/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
void w25qxx_init(w25qxx_handle_t *me, w25qxxIF_t *meIF);

void w25qxx_Reset(w25qxx_handle_t *me);
void w25qxx_Chip_Erase(w25qxx_handle_t *me);

uint32_t w25qxx_ReadID(w25qxx_handle_t *me);

void w25qxx_Read(w25qxx_handle_t *me, uint32_t startPage, uint8_t offset, uint32_t size, uint8_t *rData);
void w25qxx_FastRead(w25qxx_handle_t *me, uint32_t startPage, uint8_t offset, uint32_t size, uint8_t *rData);

void w25qxx_Erase_Sector (w25qxx_handle_t *me, uint16_t numsector);

void w25qxx_Write_Clean(w25qxx_handle_t *me, uint32_t page, uint16_t offset, uint32_t size, uint8_t *data);
void w25qxx_Write (w25qxx_handle_t *me, uint32_t page, uint16_t offset, uint32_t size, uint8_t *data);

void w25qxx_Write_Byte (w25qxx_handle_t *me, uint32_t Addr, uint8_t data);
uint8_t w25qxx_Read_Byte (w25qxx_handle_t *me, uint32_t Addr);

float w25qxx_Read_NUM (w25qxx_handle_t *me, uint32_t page, uint16_t offset);
void w25qxx_Write_NUM (w25qxx_handle_t *me, uint32_t page, uint16_t offset, float data);

void w25qxx_Read_32B (w25qxx_handle_t *me, uint32_t page, uint16_t offset, uint32_t size, uint32_t *data);
void w25qxx_Write_32B (w25qxx_handle_t *me, uint32_t page, uint16_t offset, uint32_t size, uint32_t *data);

void flash_WriteMemory(w25qxx_handle_t *me, uint8_t* buffer, uint32_t address, uint32_t buffer_size);
void flash_ReadMemory (w25qxx_handle_t *me, uint32_t Addr, uint32_t Size, uint8_t* buffer);
void flash_SectorErase(w25qxx_handle_t *me, uint32_t EraseStartAddress, uint32_t EraseEndAddress);
void flash_ChipErase (w25qxx_handle_t *me);
void flash_Reset (w25qxx_handle_t *me);

#endif /* USER_DRIVERS_W25QXX_H_ */
