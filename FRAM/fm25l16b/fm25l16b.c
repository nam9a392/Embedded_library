/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include <stdlib.h>
#include "fm25l16.h"

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/

/*==================================================================================================
*                                              ENUMS
==================================================================================================*/
typedef enum{
	OP_WREN  = 0x06,  /* Write Enable Latch */
	OP_WRDI  = 0x04,  /* Reset Write Enable Latch */
	OP_RDSR  = 0x05,  /* Read Status Register */
	OP_WRSR  = 0x01,  /* Write Status Register */
	OP_READ  = 0x03,  /* Read Memory */
	OP_WRITE = 0x02   /* Write Memory */
}fm25l16b_opcode_e;
/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

void FM25L16B_Init(fm25l16b_t *me, fm25l16bIF_t *meIF)
{
	me->meIF = meIF;
}

void FM25L16B_write_enable(fm25l16b_t *me , bool enable)
{
	fm25l16bIF_t *tmpIF = me->meIF;
	tmpIF->wait_busy();
	if(true == enable){
		me->cmd[0] = OP_WREN;
	}else{
		me->cmd[0] = OP_WRDI;
	}

	tmpIF->enable();
	tmpIF->spi_write(me->cmd,1);
	tmpIF->disable();
}

bool FM25L16B_get_status(fm25l16b_t *me, fm25l16b_status_t *status)
{
	bool retVal = true;
	uint8_t i = 0;
	uint8_t status_reg = 0;
	fm25l16bIF_t *tmpIF = me->meIF;
	me->cmd[i++] = OP_RDSR;

	tmpIF->enable();
	tmpIF->spi_write(me->cmd,i);
	tmpIF->wait_busy();
	tmpIF->spi_read(&status_reg,1);
	tmpIF->wait_busy();
	tmpIF->disable();

	status->WEL  = (status_reg >> 0x01) & 0x01;
	status->BP0  = (status_reg >> 0x02) & 0x01;
	status->BP1  = (status_reg >> 0x03) & 0x01;
	status->WPEN = (status_reg >> 0x07) & 0x01;
	return retVal;
}

bool FM25L16B_set_status(fm25l16b_t *me, fm25l16b_status_t *status)
{
	bool retVal = true;
	uint8_t i = 0;
	uint8_t status_reg = 0;
	status_reg = (status->WEL & 0x01 << 0x01) | (status->BP0 & 0x01 << 0x02) | (status->BP1 & 0x01 << 0x03) | (status->WPEN & 0x01 << 0x07);
	fm25l16bIF_t *tmpIF = me->meIF;
	me->cmd[i++] = OP_WRSR;
	me->cmd[i++] = status_reg;

	tmpIF->enable();
	tmpIF->spi_write(me->cmd,i);
	tmpIF->wait_busy();
	tmpIF->disable();
	return retVal;
}

bool FM25L16B_write(fm25l16b_t *me, uint16_t address, uint8_t *buffer, uint32_t numByte)
{
	bool retVal = true;
	uint8_t i = 0;
	fm25l16bIF_t *tmpIF = me->meIF;

	if(address > FM25L16B_ADDRESS_MAX){
		return false;
	}
	address = address & 0x7FF;
	me->cmd[i++] = OP_WRITE;
	me->cmd[i++] = (uint8_t)(address >> 8);
	me->cmd[i++] = (uint8_t)(address & 0xFF);

	tmpIF->enable();
	tmpIF->spi_write(me->cmd,i);
	tmpIF->wait_busy();
	tmpIF->spi_write(buffer,numByte);
	tmpIF->wait_busy();
	tmpIF->disable();

	return retVal;
}

bool FM25L16B_read(fm25l16b_t *me, uint16_t address, uint8_t *buffer, uint32_t numByte)
{
	bool retVal = true;
	uint8_t i = 0;
	fm25l16bIF_t *tmpIF = me->meIF;

	if(address > FM25L16B_ADDRESS_MAX){
		return false;
	}
	address = address & 0x7FF;
	me->cmd[i++] = OP_READ;
	me->cmd[i++] = (uint8_t)(address >> 8);
	me->cmd[i++] = (uint8_t)(address & 0xFF);

	tmpIF->enable();
	tmpIF->spi_write(me->cmd,i);
	tmpIF->wait_busy();
	tmpIF->spi_read(buffer,numByte);
	tmpIF->wait_busy();
	tmpIF->disable();

	return retVal;
}
