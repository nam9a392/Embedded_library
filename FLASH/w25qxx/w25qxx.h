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
#define W25Q_BLOCK_SIZE          0x10000   /* blocks of 64KBytes */
#define W25Q_SECTOR_SIZE         0x1000    /* 4kBytes */
#define W25Q_PAGE_SIZE           0x100     /* 256 bytes */
#define W25Q_FIRST_PAGE_ADDR     0x000000

#define W25Q_PAGE_ADDRESS(page_index)  (W25Q_PAGE_SIZE * (page_index))

// Macro tính chỉ số sector và block từ địa chỉ
#define W25Q_SECTOR_INDEX(address)    ((address) / W25Q_SECTOR_SIZE)
#define W25Q_BLOCK_INDEX(address)     ((address) / W25Q_BLOCK_SIZE)
#define W25Q_PAGE_INDEX(address)      ((address) / W25Q_PAGE_SIZE)

/***************************** W25Q16 ***************************************/
#define W25Q16_FLASH_SIZE        0x200000  /* 16Mbit =>2Mbyte */
#define W25Q16_TOTAL_PAGES       8192
#define W25Q16_MEMORY_SIZE       (W25Q16_TOTAL_PAGES * W25Q_PAGE_SIZE)      // Dung lượng bộ nhớ của W25Q16 (2 MB = 16 Mbit) 2MB = 2 * 1024 * 1024 bytes
#define W25Q16_TOTAL_SECTORS     (W25Q16_MEMORY_SIZE / W25Q16_SECTOR_SIZE)  // Số lượng sector 512 sectors
#define W25Q16_TOTAL_BLOCKS      (W25Q16_MEMORY_SIZE / W25Q_BLOCK_SIZE)     // 32 blocks
#define W25Q16_ADDR_SECTOR(x)    (W25Q_SECTOR_SIZE * x)

#define W25Q16_SECTOR_0          0x000000
#define W25Q16_SECTOR_1          0x001000
#define W25Q16_SECTOR_2          0x002000
#define W25Q16_SECTOR_3          0x003000
#define W25Q16_SECTOR_4          0x004000
#define W25Q16_SECTOR_5          0x005000
#define W25Q16_SECTOR_6          0x006000
#define W25Q16_SECTOR_7          0x007000
// ...
#define W25Q16_SECTOR_510        0x1FE000
#define W25Q16_SECTOR_511        0x1FF000

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                              ENUMS
==================================================================================================*/

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef union {
	uint8_t byte;  // Access entire register as a single byte
    struct {
        uint8_t BUSY  : 1;  // Bit 0: BUSY - Device busy status
        uint8_t WEL   : 1;  // Bit 1: WEL - Write enable latch
        uint8_t BP0   : 1;  // Bit 2: BP0 - Block protect bit 0
        uint8_t BP1   : 1;  // Bit 3: BP1 - Block protect bit 1
        uint8_t BP2   : 1;  // Bit 4: BP2 - Block protect bit 2
        uint8_t TB    : 1;  // Bit 5: TB - Top/Bottom block protect
        uint8_t SEC   : 1;  // Bit 6: SEC - Sector protect
        uint8_t SRP   : 1;  // Bit 7: SRP0 - Status register protect bit 0
    } bits;
}w25qxx_SR1_u;


typedef union {
	uint8_t byte;  // Access entire register as a single byte
    struct {
        uint8_t SRL    : 1;  // Bit 0: SRP1 - Status register protect bit 1
        uint8_t QE     : 1;  // Bit 1: QE - Quad enable
        uint8_t        : 1;  // Bit 2: Unused/reserved
        uint8_t LB1    : 1;  // Bit 3: LB1 - Security register lock bit 1
        uint8_t LB2    : 1;  // Bit 4: LB2 - Security register lock bit 2
        uint8_t LB3    : 1;  // Bit 5: LB3 - Security register lock bit 3
        uint8_t CMP    : 1;  // Bit 6: CMP - Complement protect
        uint8_t SUS    : 1;  // Bit 7: SUS - Suspend status
    } bits;
}w25qxx_SR2_u;


typedef union {
	uint8_t byte;  // Access entire register as a single byte
    struct {
        uint8_t        : 2;  // Bit 6-7: Unused/reserved
        uint8_t WPS    : 1;  // Bit 5: Write protect selection
        uint8_t        : 2;  // Bit 3-4: Unused/reserved
        uint8_t DRV0   : 1;  // Bit 2: Output driver strength bit 1
        uint8_t DRV1   : 1;  // Bit 1: Output driver strength bit 0
        uint8_t HOLD_RST   : 1;  // Bit 0: HOLD#/RST# disable/enable
    } bits;
}w25qxx_SR3_u;

typedef struct{
	void (*csHIGH)(void);
	void (*csLOW)(void);
	void (*spi_read)(uint8_t *pdata,uint16_t len);
	void (*spi_write)(uint8_t *pdata,uint16_t len);
	void (*delay_us)(uint32_t us);
	void (*delay_ms)(uint32_t ms);                                                                  /**< point to a delay_ms function address */
	void (*debug_print)(const char *const fmt, ...);                                                /**< point to a debug_print function address */
}w25qxxIF_t;

typedef struct{
	w25qxxIF_t   *meIF;
	w25qxx_SR1_u  SR1;
	w25qxx_SR2_u  SR2;
	w25qxx_SR3_u  SR3;
	uint8_t       tempBytes[4];
    uint8_t       tempData[4096];
}w25qxx_handle_t;
/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
void w25qxx_init(w25qxx_handle_t *me, w25qxxIF_t *meIF);
uint32_t w25qxx_ReadID(w25qxx_handle_t *me);

void w25qxx_Reset(w25qxx_handle_t *me);
void w25qxx_Erase_Sector (w25qxx_handle_t *me, uint16_t numsector);
void w25qxx_Chip_Erase(w25qxx_handle_t *me);

void w25qxx_Read(w25qxx_handle_t *me, uint32_t startPage, uint8_t offset, uint32_t size, uint8_t *rData);
void w25qxx_FastRead(w25qxx_handle_t *me, uint32_t startPage, uint8_t offset, uint32_t size, uint8_t *rData);
uint8_t w25qxx_Read_Byte (w25qxx_handle_t *me, uint32_t Addr);
float w25qxx_Read_NUM (w25qxx_handle_t *me, uint32_t page, uint16_t offset);
void w25qxx_Read_32B (w25qxx_handle_t *me, uint32_t page, uint16_t offset, uint32_t size, uint32_t *data);

void w25qxx_Write_Clean(w25qxx_handle_t *me, uint32_t page, uint16_t offset, uint32_t size, uint8_t *data);
void w25qxx_Write (w25qxx_handle_t *me, uint32_t page, uint16_t offset, uint32_t size, uint8_t *data);
void w25qxx_Write_Byte (w25qxx_handle_t *me, uint32_t Addr, uint8_t data);
void w25qxx_Write_NUM (w25qxx_handle_t *me, uint32_t page, uint16_t offset, float data);
void w25qxx_Write_32B (w25qxx_handle_t *me, uint32_t page, uint16_t offset, uint32_t size, uint32_t *data);

void w25qxx_PowerDown(w25qxx_handle_t *me);
void w25qxx_ReleasePowerDown(w25qxx_handle_t *me);

void flash_WriteMemory(w25qxx_handle_t *me, uint32_t address, uint8_t* buffer, uint32_t buffer_size);
void flash_ReadMemory (w25qxx_handle_t *me, uint32_t Addr, uint8_t* buffer, uint32_t Size);
void flash_SectorErase(w25qxx_handle_t *me, uint32_t EraseStartAddress, uint32_t EraseEndAddress);
void flash_ChipErase (w25qxx_handle_t *me);
void flash_Reset (w25qxx_handle_t *me);

#endif /* USER_DRIVERS_W25QXX_H_ */
