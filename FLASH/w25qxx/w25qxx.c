/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include <stdlib.h>
#include <string.h>
#include "w25qxx.h"

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define numBLOCK 32  // number of total blocks for 16Mb flash, 32x16x16 pages and 32x16x16x256 Bytes

/*==================================================================================================
*                                              ENUMS
==================================================================================================*/

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
static void w25qxx_Waitforwrite(w25qxx_handle_t *me)
{
    w25qxxIF_t *tmpIF = me->meIF;
	uint8_t tData = W25Q_READ_SR1;
	tmpIF->csLOW();
	tmpIF->spi_write(&tData, 1);
	do
	{
		tmpIF->spi_read(&tData, 1);  //keep reading status register
        tmpIF->delay_ms(5);
	} while (tData & 0x01);  // until the bit to reset
	tmpIF->csHIGH();
}

static void write_enable (w25qxx_handle_t *me)
{
    w25qxxIF_t *tmpIF = me->meIF;
	uint8_t tData = W25Q_WRITE_ENABLE;  // enable write
	tmpIF->csLOW();
	tmpIF->spi_write(&tData, 1);
	tmpIF->csHIGH();
	tmpIF->delay_ms(5);  // 5ms delay
}

static void write_disable(w25qxx_handle_t *me)
{
    w25qxxIF_t *tmpIF = me->meIF;
	uint8_t tData = W25Q_WRITE_ENABLE;  // disable write
	tmpIF->csLOW();
	tmpIF->spi_write(&tData, 1);
	tmpIF->csHIGH();
	tmpIF->delay_ms(5);  // 5ms delay
}

static uint32_t bytestowrite (uint32_t size, uint16_t offset)
{
	if ((size+offset)<256) return size;
	else return 256-offset;
}

static uint32_t bytestomodify (uint32_t size, uint16_t offset)
{
	if ((size+offset)<4096) return size;
	else return 4096-offset;
}

static void float2Bytes(uint8_t * ftoa_bytes_temp,float float_variable)
{
    union {
      float a;
      uint8_t bytes[4];
    } thing;

    thing.a = float_variable;

    for (uint8_t i = 0; i < 4; i++) {
      ftoa_bytes_temp[i] = thing.bytes[i];
    }

}

static float Bytes2float(uint8_t * ftoa_bytes_temp)
{
    union {
      float a;
      uint8_t bytes[4];
    } thing;

    for (uint8_t i = 0; i < 4; i++) {
    	thing.bytes[i] = ftoa_bytes_temp[i];
    }

   float float_variable =  thing.a;
   return float_variable;
}
/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

void w25qxx_init(w25qxx_handle_t *me, w25qxxIF_t *meIF)
{
    //ASSERT((me != NULL) && (meIF != NULL));
    me->meIF = meIF;
}

void w25qxx_Reset(w25qxx_handle_t *me)
{
    w25qxxIF_t *tmpIF = me->meIF;
    uint8_t tData[2];
	tData[0] = W25Q_ENABLE_RST;  // enable Reset
	tData[1] = W25Q_RESET;  // Reset
	tmpIF->csLOW();
	tmpIF->spi_write(tData, 2);
	tmpIF->csHIGH();
    tmpIF->delay_ms(100);
}
void w25qxx_Chip_Erase(w25qxx_handle_t *me)
{
    w25qxxIF_t *tmpIF = me->meIF;
    uint8_t tData = W25Q_CHIP_ERASE;
	uint8_t unlock_code = 0x98;

	write_enable(me);

	tmpIF->csLOW();
	tmpIF->spi_write(&unlock_code, 1);
	tmpIF->csHIGH();
	w25qxx_Waitforwrite(me);

	tmpIF->csLOW();
	tmpIF->spi_write(&tData, 1);
	tmpIF->csHIGH();

	w25qxx_Waitforwrite(me);

	write_disable(me);
}

uint32_t w25qxx_ReadID(w25qxx_handle_t *me)
{
    w25qxxIF_t *tmpIF = me->meIF;
    uint8_t tData = 0x9F;  // Read JEDEC ID
	uint8_t rData[3];
	tmpIF->csLOW();
	tmpIF->spi_write(&tData, 1);
	tmpIF->spi_read(rData, 3);
	tmpIF->csHIGH();
	return ((rData[0]<<16)|(rData[1]<<8)|rData[2]);
}

void w25qxx_Read(w25qxx_handle_t *me, uint32_t startPage, uint8_t offset, uint32_t size, uint8_t *rData)
{
    w25qxxIF_t *tmpIF = me->meIF;
    uint8_t tData[5];
	uint32_t memAddr = (startPage*256) + offset;

	if (numBLOCK<512)   // Chip Size<256Mb
	{
		tData[0] = W25Q_READ_DATA;  // enable Read
		tData[1] = (memAddr>>16)&0xFF;  // MSB of the memory Address
		tData[2] = (memAddr>>8)&0xFF;
		tData[3] = (memAddr)&0xFF; // LSB of the memory Address
	}
	else  // we use 32bit memory address for chips >= 256Mb
	{
		tData[0] = W25Q_READ_DATA_4B;  // Read Data with 4-Byte Address
		tData[1] = (memAddr>>24)&0xFF;  // MSB of the memory Address
		tData[2] = (memAddr>>16)&0xFF;
		tData[3] = (memAddr>>8)&0xFF;
		tData[4] = (memAddr)&0xFF; // LSB of the memory Address
	}

	tmpIF->csLOW();  // pull the CS Low
	if (numBLOCK<512)
	{
		tmpIF->spi_write(tData, 4);  // send read instruction along with the 24 bit memory address
	}
	else
	{
		tmpIF->spi_write(tData, 5);  // send read instruction along with the 32 bit memory address
	}

	tmpIF->spi_read(rData, size);  // Read the data
	tmpIF->csHIGH();  // pull the CS High
}

void w25qxx_FastRead(w25qxx_handle_t *me, uint32_t startPage, uint8_t offset, uint32_t size, uint8_t *rData)
{
    w25qxxIF_t *tmpIF = me->meIF;
    uint8_t tData[6];
	uint32_t memAddr = (startPage*256) + offset;

	if (numBLOCK<512)   // Chip Size<256Mb
	{
		tData[0] = W25Q_FAST_READ;  // enable Fast Read
		tData[1] = (memAddr>>16)&0xFF;  // MSB of the memory Address
		tData[2] = (memAddr>>8)&0xFF;
		tData[3] = (memAddr)&0xFF; // LSB of the memory Address
		tData[4] = 0;  // Dummy clock
	}
	else  // we use 32bit memory address for chips >= 256Mb
	{
		tData[0] = W25Q_FAST_READ_4B;  // Fast Read with 4-Byte Address
		tData[1] = (memAddr>>24)&0xFF;  // MSB of the memory Address
		tData[2] = (memAddr>>16)&0xFF;
		tData[3] = (memAddr>>8)&0xFF;
		tData[4] = (memAddr)&0xFF; // LSB of the memory Address
		tData[5] = 0;  // Dummy clock
	}

	tmpIF->csLOW();  // pull the CS Low
	if (numBLOCK<512)
	{
		tmpIF->spi_write(tData, 5);  // send read instruction along with the 24 bit memory address
	}
	else
	{
		tmpIF->spi_write(tData, 6);  // send read instruction along with the 32 bit memory address
	}

	tmpIF->spi_read(rData, size);  // Read the data
	tmpIF->csHIGH();  // pull the CS High
}

void w25qxx_Erase_Sector(w25qxx_handle_t *me, uint16_t numsector)
{
    w25qxxIF_t *tmpIF = me->meIF;
    uint8_t tData[6];
	uint32_t memAddr = numsector*16*256;   // Each sector contains 16 pages * 256 bytes

	write_enable(me);

	if (numBLOCK<512)   // Chip Size<256Mb
	{
		tData[0] = W25Q_SECTOR_ERASE;  // Erase sector
		tData[1] = (memAddr>>16)&0xFF;  // MSB of the memory Address
		tData[2] = (memAddr>>8)&0xFF;
		tData[3] = (memAddr)&0xFF; // LSB of the memory Address

		tmpIF->csLOW();
		tmpIF->spi_write(tData, 4);
		tmpIF->csHIGH();
	}
	else  // we use 32bit memory address for chips >= 256Mb
	{
		tData[0] = W25Q_SECTOR_ERASE_4B;  // ERASE Sector with 32bit address
		tData[1] = (memAddr>>24)&0xFF;
		tData[2] = (memAddr>>16)&0xFF;
		tData[3] = (memAddr>>8)&0xFF;
		tData[4] = memAddr&0xFF;

		tmpIF->csLOW();  // pull the CS LOW
		tmpIF->spi_write(tData, 5);
		tmpIF->csHIGH();  // pull the HIGH
	}

	w25qxx_Waitforwrite(me);

	write_disable(me);
}

void w25qxx_Write_Clean(w25qxx_handle_t *me, uint32_t page, uint16_t offset, uint32_t size, uint8_t *data)
{
    w25qxxIF_t *tmpIF = me->meIF;
    uint8_t tData[266];
	uint32_t startPage = page;
	uint32_t endPage  = startPage + ((size+offset-1)/256);
	uint32_t numPages = endPage-startPage+1;

	uint16_t startSector  = startPage/16;
	uint16_t endSector  = endPage/16;
	uint16_t numSectors = endSector-startSector+1;
	for (uint16_t i=0; i<numSectors; i++)
	{
		w25qxx_Erase_Sector(me, startSector++);
	}

	uint32_t dataPosition = 0;

	// write the data
	for (uint32_t i=0; i<numPages; i++)
	{
		uint32_t memAddr = (startPage*256)+offset;
		uint16_t bytesremaining  = bytestowrite(size, offset);
		uint32_t indx = 0;

		write_enable(me);

		if (numBLOCK<512)   // Chip Size<256Mb
		{
			tData[0] = W25Q_PAGE_PROGRAM;  // page program
			tData[1] = (memAddr>>16)&0xFF;  // MSB of the memory Address
			tData[2] = (memAddr>>8)&0xFF;
			tData[3] = (memAddr)&0xFF; // LSB of the memory Address

			indx = 4;
		}

		else // we use 32bit memory address for chips >= 256Mb
		{
			tData[0] = W25Q_PAGE_PROGRAM_4B;  // page program with 4-Byte Address
			tData[1] = (memAddr>>24)&0xFF;  // MSB of the memory Address
			tData[2] = (memAddr>>16)&0xFF;
			tData[3] = (memAddr>>8)&0xFF;
			tData[4] = (memAddr)&0xFF; // LSB of the memory Address

			indx = 5;
		}

		uint16_t bytestosend  = bytesremaining + indx;

		for (uint16_t i=0; i<bytesremaining; i++)
		{
			tData[indx++] = data[i+dataPosition];
		}

		if (bytestosend > 250)
		{
			tmpIF->csLOW();
			tmpIF->spi_write(tData, 100);
			tmpIF->spi_write(tData+100, bytestosend-100);
			tmpIF->csHIGH();

		}

		else
		{
			tmpIF->csLOW();
			tmpIF->spi_write(tData, bytestosend);
			tmpIF->csHIGH();
		}


		startPage++;
		offset = 0;
		size = size-bytesremaining;
		dataPosition = dataPosition+bytesremaining;

		w25qxx_Waitforwrite(me);
		write_disable(me);

	}
}

void w25qxx_Write (w25qxx_handle_t *me, uint32_t page, uint16_t offset, uint32_t size, uint8_t *data)
{
    w25qxxIF_t *tmpIF = me->meIF;
    uint16_t startSector  = page/16;
	uint16_t endSector  = (page + ((size+offset-1)/256))/16;
	uint16_t numSectors = endSector-startSector+1;

	uint8_t previousData[4096];
	uint32_t sectorOffset = ((page%16)*256)+offset;
	uint32_t dataindx = 0;

	for (uint16_t i=0; i<numSectors; i++)
	{
		uint32_t startPage = startSector*16;
		w25qxx_FastRead(me, startPage, 0, 4096, previousData);

		uint16_t bytesRemaining = bytestomodify(size, sectorOffset);
		for (uint16_t i=0; i<bytesRemaining; i++)
		{
			previousData[i+sectorOffset] = data[i+dataindx];
		}

		w25qxx_Write_Clean(me, startPage, 0, 4096, previousData);

		startSector++;
		sectorOffset = 0;
		dataindx = dataindx+bytesRemaining;
		size = size-bytesRemaining;
	}
}

void w25qxx_Write_Byte (w25qxx_handle_t *me, uint32_t Addr, uint8_t data)
{
    w25qxxIF_t *tmpIF = me->meIF;
	uint8_t tData[6];
	uint8_t indx;

	if (numBLOCK<512)   // Chip Size<256Mb
	{
		tData[0] = W25Q_PAGE_PROGRAM;  // page program
		tData[1] = (Addr>>16)&0xFF;  // MSB of the memory Address
		tData[2] = (Addr>>8)&0xFF;
		tData[3] = (Addr)&0xFF; // LSB of the memory Address
		tData[4] = data;
		indx = 5;
	}
	else  // we use 32bit memory address for chips >= 256Mb
	{
		tData[0] = W25Q_PAGE_PROGRAM_4B;  // Write Data with 4-Byte Address
		tData[1] = (Addr>>24)&0xFF;  // MSB of the memory Address
		tData[2] = (Addr>>16)&0xFF;
		tData[3] = (Addr>>8)&0xFF;
		tData[4] = (Addr)&0xFF; // LSB of the memory Address
		tData[5] = data;
		indx = 6;
	}


	if (w25qxx_Read_Byte(me, Addr) == 0xFF)
	{
		write_enable(me);
		tmpIF->csLOW();
		tmpIF->spi_write(tData, indx);
		tmpIF->csHIGH();

		w25qxx_Waitforwrite(me);
		write_disable(me);
	}
}

uint8_t w25qxx_Read_Byte (w25qxx_handle_t *me, uint32_t Addr)
{
    w25qxxIF_t *tmpIF = me->meIF;
    uint8_t tData[5];
	uint8_t rData;

	if (numBLOCK<512)   // Chip Size<256Mb
	{
		tData[0] = W25Q_READ_DATA;  // enable Read
		tData[1] = (Addr>>16)&0xFF;  // MSB of the memory Address
		tData[2] = (Addr>>8)&0xFF;
		tData[3] = (Addr)&0xFF; // LSB of the memory Address
	}
	else  // we use 32bit memory address for chips >= 256Mb
	{
		tData[0] = W25Q_READ_DATA_4B;  // Read Data with 4-Byte Address
		tData[1] = (Addr>>24)&0xFF;  // MSB of the memory Address
		tData[2] = (Addr>>16)&0xFF;
		tData[3] = (Addr>>8)&0xFF;
		tData[4] = (Addr)&0xFF; // LSB of the memory Address
	}

	tmpIF->csLOW();  // pull the CS Low
	if (numBLOCK<512)
	{
		tmpIF->spi_write(tData, 4);  // send read instruction along with the 24 bit memory address
	}
	else
	{
		tmpIF->spi_write(tData, 5);  // send read instruction along with the 32 bit memory address
	}

	tmpIF->spi_read(&rData, 1);  // Read the data
	tmpIF->csHIGH();  // pull the CS High

	return rData;
}

float w25qxx_Read_NUM (w25qxx_handle_t *me, uint32_t page, uint16_t offset)
{
	uint8_t rData[4];
	w25qxx_Read(me, page, offset, 4, rData);
	return (Bytes2float(rData));
}

void w25qxx_Write_NUM (w25qxx_handle_t *me, uint32_t page, uint16_t offset, float data)
{
    float2Bytes(me->tempBytes, data);
    w25qxx_Write(me, page, offset, 4, me->tempBytes);
}


void w25qxx_Read_32B (w25qxx_handle_t *me, uint32_t page, uint16_t offset, uint32_t size, uint32_t *data)
{
    uint8_t data8[size*4];
	uint32_t indx = 0;

	w25qxx_FastRead(me, page, offset, size*4, data8);

	for (uint32_t i=0; i<size; i++)
	{
		data[i] = (data8[indx++]) | (data8[indx++]<<8) | (data8[indx++]<<16) | (data8[indx++]<<24);
	}
}

void w25qxx_Write_32B (w25qxx_handle_t *me, uint32_t page, uint16_t offset, uint32_t size, uint32_t *data)
{
    uint8_t data8[size*4];
	uint32_t indx = 0;

	for (uint32_t i=0; i<size; i++)
	{
		data8[indx++] = data[i]&0xFF;   // extract LSB
		data8[indx++] = (data[i]>>8)&0xFF;
		data8[indx++] = (data[i]>>16)&0xFF;
		data8[indx++] = (data[i]>>24)&0xFF;
	}

	w25qxx_Write(me, page, offset, indx, data8);
}

void flash_WriteMemory(w25qxx_handle_t *me, uint8_t* buffer, uint32_t address, uint32_t buffer_size)
{
    w25qxxIF_t *tmpIF = me->meIF;
    uint32_t page = address/256;
	uint16_t offset = address%256;
	uint32_t size = buffer_size;
	uint8_t tData[266];
	uint32_t startPage = page;
	uint32_t endPage  = startPage + ((size+offset-1)/256);
	uint32_t numPages = endPage-startPage+1;

	uint32_t dataPosition = 0;

	// write the data
	for (uint32_t i=0; i<numPages; i++)
	{
		uint32_t memAddr = (startPage*256)+offset;
		uint16_t bytesremaining  = bytestowrite(size, offset);
		uint32_t indx = 0;

		write_enable(me);

		if (numBLOCK<512)   // Chip Size<256Mb
		{
			tData[0] = W25Q_PAGE_PROGRAM;  // page program
			tData[1] = (memAddr>>16)&0xFF;  // MSB of the memory Address
			tData[2] = (memAddr>>8)&0xFF;
			tData[3] = (memAddr)&0xFF; // LSB of the memory Address

			indx = 4;
		}

		else // we use 32bit memory address for chips >= 256Mb
		{
			tData[0] = W25Q_PAGE_PROGRAM_4B;  // page program with 4-Byte Address
			tData[1] = (memAddr>>24)&0xFF;  // MSB of the memory Address
			tData[2] = (memAddr>>16)&0xFF;
			tData[3] = (memAddr>>8)&0xFF;
			tData[4] = (memAddr)&0xFF; // LSB of the memory Address

			indx = 5;
		}

		uint16_t bytestosend  = bytesremaining + indx;

		for (uint16_t i=0; i<bytesremaining; i++)
		{
			tData[indx++] = buffer[i+dataPosition];
		}

		if (bytestosend > 250)
		{
			tmpIF->csLOW();
			tmpIF->spi_write(tData, 100);
			tmpIF->spi_write(tData+100, bytestosend-100);
			tmpIF->csHIGH();

		}

		else
		{
			tmpIF->csLOW();
			tmpIF->spi_write(tData, bytestosend);
			tmpIF->csHIGH();
		}


		startPage++;
		offset = 0;
		size = size-bytesremaining;
		dataPosition = dataPosition+bytesremaining;

		w25qxx_Waitforwrite(me);
		write_disable(me);

	}

}

void flash_ReadMemory (w25qxx_handle_t *me, uint32_t Addr, uint32_t Size, uint8_t* buffer)
{
    uint32_t page = Addr/256;
	uint16_t offset = Addr%256;

	w25qxx_FastRead(me, page, offset, Size, buffer);
}

void flash_SectorErase(w25qxx_handle_t *me, uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
    uint16_t startSector  = EraseStartAddress/4096;
	uint16_t endSector  = EraseEndAddress/4096;
	uint16_t numSectors = endSector-startSector+1;
	for (uint16_t i=0; i<numSectors; i++)
	{
		w25qxx_Erase_Sector(me, startSector++);
	}
}

void flash_ChipErase (w25qxx_handle_t *me)
{
    w25qxx_Chip_Erase(me);
}

void flash_Reset (w25qxx_handle_t *me)
{
    w25qxx_Reset(me);
}
