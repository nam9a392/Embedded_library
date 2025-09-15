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
/**
 * @brief w25qxx information structure definition
 */
typedef struct w25qxx_info_s
{
    char chip_name[32];                /**< chip name */
    char manufacturer_name[32];        /**< manufacturer name */
    char interface[16];                /**< chip interface name */
    float supply_voltage_min_v;        /**< chip min supply voltage */
    float supply_voltage_max_v;        /**< chip max supply voltage */
    float max_current_ma;              /**< chip max current */
    float temperature_min;             /**< chip min operating temperature */
    float temperature_max;             /**< chip max operating temperature */
    uint32_t driver_version;           /**< driver version */
} w25qxx_info_t;

typedef struct{
	void (*enable)(void);
	void (*disable)(void);
	void (*spi_read)(uint8_t *pdata,uint16_t len);
	void (*spi_write)(uint8_t *pdata,uint16_t len);
	void (*wait_busy)(void);
	void (*delay_ms)(uint32_t ms);                                                                     /**< point to a delay_ms function address */
	void (*delay_us)(uint32_t ms);                                                                     /**< point to a delay_us function address */
	void (*debug_print)(const char *const fmt, ...);                                                   /**< point to a debug_print function address */
}w25qxxIF_t;

typedef struct{
	uint8_t      cmd[10];
	w25q16IF_t   *meIF;
    void (*debug_print)(const char *const fmt, ...);                                                   /**< point to a debug_print function address */
    uint8_t inited;                                                                                    /**< inited flag */
    uint16_t type;                                                                                     /**< chip type */
    uint8_t address_mode;                                                                              /**< address mode */
    uint8_t param;                                                                                     /**< param */
    uint8_t dummy;                                                                                     /**< dummy */
    uint8_t dual_quad_spi_enable;                                                                      /**< dual spi and quad spi enable */
    uint8_t spi_qspi;                                                                                  /**< spi qspi interface type */
    uint8_t buf[256 + 6];                                                                              /**< inner buffer */
    uint8_t buf_4k[4096 + 1];                                                                          /**< 4k inner buffer */
}w25qxx_handle_t;
/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/

/**
 * @}
 */

/**
 * @defgroup w25qxx_basic_driver w25qxx basic driver function
 * @brief    w25qxx basic driver modules
 * @ingroup  w25qxx_driver
 * @{
 */

/**
 * @brief      get chip's information
 * @param[out] *info pointer to a w25qxx info structure
 * @return     status code
 *             - 0 success
 *             - 2 handle is NULL
 * @note       none
 */
uint8_t w25qxx_info(w25qxx_info_t *info);

/**
 * @brief     initialize the chip
 * @param[in] *handle pointer to a w25qxx handle structure
 * @return    status code
 *            - 0 success
 *            - 1 spi or qspi initialization failed
 *            - 2 handle is NULL
 *            - 3 linked functions is NULL
 *            - 4 get manufacturer device id failed
 *            - 5 enter qspi failed
 *            - 6 id is invalid
 *            - 7 reset failed
 *            - 8 set address mode failed
 * @note      none
 */
uint8_t w25qxx_init(w25qxx_handle_t *handle, w25qxxIF_t *meIF);

/**
 * @brief     close the chip
 * @param[in] *handle pointer to a w25qxx handle structure
 * @return    status code
 *            - 0 success
 *            - 1 spi or qspi deinit failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 *            - 4 power down failed
 * @note      none
 */
uint8_t w25qxx_deinit(w25qxx_handle_t *handle);


#endif /* USER_DRIVERS_W25QXX_H_ */
