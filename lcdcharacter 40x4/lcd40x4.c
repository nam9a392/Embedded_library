/*
 * lcd40x4.c
 *
 *  Created on: Sep 5, 2023
 *      Author: ADMIN
 */


/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include <string.h>
#include "Lcd40x4.h"
/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/

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
/**
 * @brief  This function uses to initialize the LCD to work in 4 bits mode
 *
 * @param[in]  None
 *
 * @retval void
 *
 */

static void Lcd4004_data_Write(Lcd4004 *me, uint8_t data);
static void Lcd4004_command_Write(Lcd4004 *me, uint8_t cmd);

#if LCD_OPERATION_MODE	== LCD_4_BIT_MODE
static void Lcd4004_4bits_Mode_Init(Lcd4004 *me);
#elif LCD_OPERATION_MODE	== LCD_8_BIT_MODE
static void Lcd4004_8bits_Mode_Init(Lcd4004 *me);
#endif
/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/
/**
 *
 * @implement Lcd_Write_Activity
 *
 */


static void Lcd4004_data_Write(Lcd4004 *me, uint8_t data)
{
    //ASSERT(me!=NULL);
    Lcd4004IF *tmpIF = me->meIF;
    tmpIF->rw(0);
#if(LCD_OPERATION_MODE == LCD_4_BIT_MODE)
	/*Send 4 bit High of command*/
	tmpIF->write_4bit(data >> 4);
	/*Send 4 bit Low of command*/
	tmpIF->write_4bit(data & 0xF);
#elif (LCD_OPERATION_MODE == LCD_8_BIT_MODE)
	tmpIF->write_8bit(data);
#endif
	if(LCD_CHIP_0 == me->chip)
	{
		tmpIF->enable_1();
	}else if(LCD_CHIP_1 == me->chip)
	{
		tmpIF->enable_2();
	}
}

static void Lcd4004_command_Write(Lcd4004 *me, uint8_t cmd)
{
	//ASSERT(me!=NULL);
	Lcd4004IF *tmpIF = me->meIF;
    /*RS = 0, for LCD command*/
	tmpIF->instruction_mode();
	Lcd4004_data_Write(me,cmd);

}

#if(LCD_OPERATION_MODE == LCD_4_BIT_MODE)
static void Lcd4004_4bits_Mode_Init(Lcd4004 *me)
{
	//ASSERT(me!=NULL);
	Lcd4004IF *tmpIF = me->meIF;
    //1. Do the lcd initialization
	tmpIF->delay_us(40);
    /*RS = 0, for LCD command*/
	tmpIF->instruction_mode();

    tmpIF->write_4bit(0x3U);
    tmpIF->delay_us(5);
    tmpIF->write_4bit(0x3U);
    tmpIF->delay_us(150);

    tmpIF->write_4bit(0x3U);
    tmpIF->write_4bit(0x2U);

    //function set command
    Lcd4004_command_Write(me, LCD_CMD_4DL_2N_5X8F);
    tmpIF->delay_us(40);

    //Display on cursor on
    Lcd4004_command_Write(me, LCD_CMD_DON_CUROFF);
    tmpIF->delay_us(40);

    Lcd4004_Clear(me);
    //entry mode set
    Lcd4004_command_Write(me, LCD_CMD_INCADD);
    tmpIF->delay_us(40);

}
#elif (LCD_OPERATION_MODE == LCD_8_BIT_MODE)
static void Lcd4004_8bits_Mode_Init(Lcd4004 *me)
{
	//ASSERT(me!=NULL);
	Lcd4004IF *tmpIF = me->meIF;
    //1. Do the lcd initialization
	tmpIF->delay_us(40);
    /*RS = 0, for LCD command*/
	tmpIF->instruction_mode();

    Lcd4004_command_Write(me, 0x30U);
    tmpIF->delay_us(5);
    Lcd4004_command_Write(me, 0x30U);
    tmpIF->delay_us(200);

    Lcd4004_command_Write(me, 0x30U);

    //function set command
    Lcd4004_command_Write(me, LCD_CMD_8DL_2N_5X8F);
    tmpIF->delay_us(40);

    //Display on cursor off
    me->displaycontrol = LCD_CMD_DON_CUROFF;
    Lcd4004_command_Write(me, LCD_CMD_DON_CUROFF);
    tmpIF->delay_us(40);

    //Lcd_Clear();
    //entry mode set
    Lcd4004_command_Write(me, LCD_CMD_INCADD);
    tmpIF->delay_us(40);

}
#endif
/*==================================================================================================
*                                        GLOBAL FUNCTIONS
==================================================================================================*/
/*************************** Basic functions **********************************/
void Lcd4004_init(Lcd4004 *me, Lcd4004IF *meIF)
{
	//ASSERT((me!=NULL)&&(meIF!=NULL));
    me->meIF = meIF;
#if(LCD_OPERATION_MODE == LCD_4_BIT_MODE)
    me->chip = LCD_CHIP_1;
    Lcd4004_4bits_Mode_Init(me);
    me->chip = LCD_CHIP_0;
    Lcd4004_4bits_Mode_Init(me);
#elif (LCD_OPERATION_MODE == LCD_8_BIT_MODE)
	me->chip = LCD_CHIP_1;
	Lcd4004_8bits_Mode_Init(me);
	me->chip = LCD_CHIP_0;
	Lcd4004_8bits_Mode_Init(me);
#endif
	Lcd4004_Clear(me);
//	Lcd_Return_Home(me);
}

void Lcd4004_Clear(Lcd4004 *me)
{
	//ASSERT(me!=NULL);
	Lcd4004IF *tmpIF = me->meIF;
    /*write clear command from ic1 to ic0 respectively cursor will stop at pos 0 of ic0 */
    me->chip = LCD_CHIP_1;
    Lcd4004_command_Write(me, LCD_CMD_DIS_CLEAR);
    me->chip = LCD_CHIP_0;
    Lcd4004_command_Write(me, LCD_CMD_DIS_CLEAR);
    /*delay more than 1.52ms for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_ms(LCD_CLEAR_CMD_DELAY);
    //Lcd_Set_Cursor(me,0,0);
}

void Lcd4004_Return_Home(Lcd4004 *me)
{
	//ASSERT(me!=NULL);
	Lcd4004IF *tmpIF = me->meIF;
	me->chip = LCD_CHIP_1;
    Lcd4004_command_Write(me, LCD_CMD_DIS_RETURN_HOME);
    me->chip = LCD_CHIP_0;
    Lcd4004_command_Write(me, LCD_CMD_DIS_RETURN_HOME);
    tmpIF->delay_ms(LCD_RETURN_HOME_CMD_DELAY);
    //Lcd_Set_Cursor(me,0,0);
}


/*
 * This function sends a character to the LCD
 * Here we are using 4 bits parallel data transmission
 * */
void Lcd4004_Put_Char(Lcd4004 *me, uint8_t data)
{
	//ASSERT(me!=NULL);
	Lcd4004IF *tmpIF = me->meIF;
    /*RS = 1, for LCD user data*/
    tmpIF->data_mode();
    Lcd4004_data_Write(me, data);
    tmpIF->delay_us(LCD_SET_RAM_CMD_DELAY);
}

/* if Line/col equal by 0xff print at current posistion*/
void Lcd4004_Put_String(Lcd4004 *me, uint8_t line, uint8_t offset, uint8_t *pString, uint16_t length)
{
	//ASSERT(me!=NULL);
	Lcd4004IF *tmpIF = me->meIF;
    uint16_t i=0;
    /* Check input validation */
    if((line > LCD_CHARACTER_LINES - 1) || (offset > MAX_CHARACTER_OF_LINE - 1))
    {
        return;
    }
    if((line != LCD_CURRENT_POSITION) && (offset != LCD_CURRENT_POSITION))
    {
        Lcd4004_Set_Cursor(me, line,offset);
    }
    for (i=0 ; i<length ; i++)
    {
        Lcd4004_Put_Char(me, (uint8_t)pString[i]);
    }
}

void Lcd4004_Set_Cursor(Lcd4004 *me, uint8_t line, uint8_t offset)
{
	//ASSERT(me!=NULL);
	Lcd4004IF *tmpIF = me->meIF;
    /*check input position valid*/
    if((line > LCD_CHARACTER_LINES - 1) || (offset > MAX_CHARACTER_OF_LINE - 1))
    {
        return;
    }

    if(me->chip != (line & 0b10))
    {
    	Lcd4004_command_Write(me, LCD_DISPLAYCONTROL | (me->displaycontrol & LCD_CURSORS_MASK));  //turn off cursors on chip we are leaving
    	me->chip = line & 0b10;									//if it is row 0 or 1 this is 0; if it is row 2 or 3 this is 2
    	Lcd4004_command_Write(me, me->displaycontrol);					//turn on cursor on chip we moved to
    }
    switch(line & 0x1)
    {
        case 0:
            Lcd4004_command_Write(me, offset | 0x80);
            break;
        case 1:
            Lcd4004_command_Write(me, offset | 0xC0);
            break;
        default:
            break;
    }
    /*delay more than 37us for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}

void Lcd4004_Move_Cursor_Right(Lcd4004 *me)
{
	//ASSERT(me!=NULL);
	Lcd4004IF *tmpIF = me->meIF;

	/*shift cursor position to the right*/
	Lcd4004_command_Write(me, LCD_CURSOR_MOVE|LCD_SHIFT_RIGHT);
	tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}

void Lcd4004_Move_Cursor_Left(Lcd4004 *me)
{
	//ASSERT(me!=NULL);
	Lcd4004IF *tmpIF = me->meIF;

	/*shift cursor position to the left*/
	Lcd4004_command_Write(me, LCD_CURSOR_MOVE|LCD_SHIFT_LEFT);
	tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}

void Lcd4004_Cursor_Effect(Lcd4004 *me, uint8_t CursorEffect)
{
	//ASSERT(me!=NULL);
	Lcd4004IF *tmpIF = me->meIF;
    switch(CursorEffect)
    {
        case 0:
        	me->displaycontrol = LCD_CMD_DON_CUROFF;
            Lcd4004_command_Write(me, LCD_CMD_DON_CUROFF);
            break;
        case 1:
        	me->displaycontrol = LCD_CMD_DON_CURON;
            Lcd4004_command_Write(me, LCD_CMD_DON_CURON);
            break;
        case 2:
        	me->displaycontrol = LCD_CMD_DON_CURBLINK;
            Lcd4004_command_Write(me, LCD_CMD_DON_CURBLINK);
            break;
        default:
        	me->displaycontrol = LCD_CMD_DON_CUROFF;
            Lcd4004_command_Write(me, LCD_CMD_DON_CUROFF);
            break;
    }
    /*delay more than 37us for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}

void Lcd4004_Cursor_On(Lcd4004 *me)
{
	//ASSERT(me!=NULL);
	Lcd4004IF *tmpIF = me->meIF;
    Lcd4004_command_Write(me, LCD_CMD_DON_CURON);
    /*delay more than 37us for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}

void Lcd4004_Cursor_Off(Lcd4004 *me)
{
	//ASSERT(me!=NULL);
	Lcd4004IF *tmpIF = me->meIF;
    Lcd4004_command_Write(me, LCD_CMD_DON_CUROFF);
    /*delay more than 37us for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}

void Lcd4004_Cursor_Blink(Lcd4004 *me)
{
    //ASSERT(me!=NULL);
    Lcd4004IF *tmpIF = me->meIF;
    Lcd4004_command_Write(me, LCD_CMD_DON_CURBLINK);
    /*delay more than 37us for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}

void Lcd4004_Set_Contrast(Lcd4004 *me,uint32_t contrastValue)
{
    //ASSERT(me!=NULL);
    Lcd4004IF *tmpIF = me->meIF;
    tmpIF->contrast(contrastValue);
}
/*************************** Extended functions **********************************/
