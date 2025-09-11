/*
 * fm25l16b.h
 *
 *  Created on: Aug 10, 2024
 *      Author: HUNGNT85
 */

#ifndef USER_DRIVERS_FM25L16B_H_
#define USER_DRIVERS_FM25L16B_H_

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include <stdint.h>

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/

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
	bool WEL;  // Write Enable Latch
	bool BP0;  // Block Protect bit ‘0’
	bool BP1;  // Block Protect bit ‘1’
	bool WPEN; // Write Protect Enable bit
}fm25l16b_status_t;

typedef struct{
	void (*enable)(void);
	void (*disable)(void);
	void (*spi_read)(uint8_t *pdata,uint16_t len);
	void (*spi_write)(uint8_t *pdata,uint16_t len);
	void (*wait_busy)(void);
}w25q16IF_t;

typedef struct{
	uint8_t      cmd[10];
	w25q16IF_t   *meIF;
}w25q16_t;
/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
void W25Q16_Init(fm25l16b_t *me, fm25l16bIF_t *meIF);
void W25Q16_write_enable(fm25l16b_t *me);
bool W25Q16_get_status(fm25l16b_t *me, fm25l16b_status_t *status);
bool W25Q16_set_status(fm25l16b_t *me, fm25l16b_status_t *status);
void W25Q16_write(fm25l16b_t *me, uint16_t address, uint8_t *buffer, uint32_t numByte);
void W25Q16_read(fm25l16b_t *me, uint16_t address, uint8_t *buffer, uint32_t numByte);

#endif /* USER_DRIVERS_FM25L16B_H_ */
