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
#define FM25L16B_ADDRESS_MAX          0x7FF
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
}fm25l16bIF_t;

typedef struct{
	uint8_t      cmd[10];
	fm25l16bIF_t *meIF;
}fm25l16b_t;
/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
void FM25L16B_Init(fm25l16b_t *me, fm25l16bIF_t *meIF);
void FM25L16B_write_enable(fm25l16b_t *me);
bool FM25L16B_get_status(fm25l16b_t *me, fm25l16b_status_t *status);
bool FM25L16B_set_status(fm25l16b_t *me, fm25l16b_status_t *status);
void FM25L16B_write(fm25l16b_t *me, uint16_t address, uint8_t *buffer, uint32_t numByte);
void FM25L16B_read(fm25l16b_t *me, uint16_t address, uint8_t *buffer, uint32_t numByte);

#endif /* USER_DRIVERS_FM25L16B_H_ */
