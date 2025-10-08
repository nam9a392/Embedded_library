#ifndef USER_DRIVERS_FM25L16_H_
#define USER_DRIVERS_FM25L16_H_

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include <stdint.h>
#include <stdbool.h>
/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define FM25L16B_ADDRESS_MIN          0x000
#define FM25L16B_ADDRESS_MAX          0x7FF  // 2KB data size
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
	void (*debug_print)(const char *const fmt, ...);
}fm25l16bIF_t;

typedef struct{
	uint8_t      cmd[10];
	fm25l16bIF_t *meIF;
	fm25l16b_status_t status;
}fm25l16b_t;
/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
void fm25l16b_Init(fm25l16b_t *me, fm25l16bIF_t *meIF);
void fm25l16b_write_enable(fm25l16b_t *me , bool enable);
bool fm25l16b_get_status(fm25l16b_t *me, fm25l16b_status_t *status);
bool fm25l16b_set_status(fm25l16b_t *me, fm25l16b_status_t *status);
bool fm25l16b_write(fm25l16b_t *me, uint16_t address, uint8_t *buffer, uint32_t numByte);
bool fm25l16b_read(fm25l16b_t *me, uint16_t address, uint8_t *buffer, uint32_t numByte);

#endif /* USER_DRIVERS_FM25L16_H_ */