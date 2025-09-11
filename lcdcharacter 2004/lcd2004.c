/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "Lcd2004.h"
#include <string.h>
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
static void Lcd2004_4bits_Mode_Init(Lcd2004 *me);
static void Lcd2004_data_write(Lcd2004 *me, uint8_t data);
static void Lcd2004_Display_Move_Right(Lcd2004 *me);
static void Lcd2004_Display_Move_Left(Lcd2004 *me);
/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/
static void Lcd2004_data_write(Lcd2004 *me, uint8_t data)
{
    //ASSERT(me!=NULL);
    Lcd2004IF *tmpIF = me->meIF;
#if LCD_OPERATION_MODE	== LCD_8_BIT_MODE
    tmpIF->write_8bit(data);
    tmpIF->enable();
    /*Send 4 bit High of command*/
#elif LCD_OPERATION_MODE	== LCD_4_BIT_MODE
	tmpIF->write_4bit(data >> 4);
	tmpIF->enable();
    tmpIF->write_4bit(data & 0xF);
    tmpIF->enable();
#endif
}

static void Lcd2004_command_write(Lcd2004 *me, uint8_t cmd)
{
    //ASSERT(me!=NULL);
    Lcd2004IF *tmpIF = me->meIF;
    /*RS = 0, for LCD command*/
    tmpIF->instruction_mode();
    Lcd2004_data_write(me, cmd);

}

#if LCD_OPERATION_MODE	== LCD_4_BIT_MODE
static void Lcd2004_4bits_Mode_Init(Lcd2004 *me)
{
    //ASSERT(me!=NULL);
    Lcd2004IF *tmpIF = me->meIF;
    //1. Do the lcd initialization
    tmpIF->delay_ms(40);
    /*RS = 0, for LCD command*/
    tmpIF->instruction_mode();

    tmpIF->write_4bit(0x3U);
    tmpIF->enable();
    tmpIF->delay_us(5);
    tmpIF->write_4bit(0x3U);
    tmpIF->enable();
    tmpIF->delay_us(150);

    tmpIF->write_4bit(0x3U);
    tmpIF->enable();
    tmpIF->write_4bit(0x2U);
    tmpIF->enable();
    //function set command
    Lcd2004_command_write(me,LCD_CMD_4DL_2N_5X8F);
    tmpIF->delay_us(40);

    //Display on cursor on
    Lcd2004_command_write(me,LCD_CMD_DON_CURON);
    tmpIF->delay_us(40);

    //Lcd2004_Clear(me);
    //entry mode set
    Lcd2004_command_write(me,LCD_CMD_INCADD);
    tmpIF->delay_us(40);
}
#elif LCD_OPERATION_MODE	== LCD_8_BIT_MODE
static void Lcd2004_8bits_Mode_Init(Lcd2004 *me)
{
    //ASSERT(me!=NULL);
    Lcd2004IF *tmpIF = me->meIF;
    //1. Do the lcd initialization
    tmpIF->delay_us(40);
    /*RS = 0, for LCD command*/
    tmpIF->instruction_mode();

    Lcd2004_command_write(me,0x30U);
    tmpIF->delay_us(5);
    Lcd2004_command_write(me,0x30U);
    tmpIF->delay_us(200);

    Lcd2004_command_write(me,0x30U);

    //function set command
    Lcd2004_command_write(me,LCD_CMD_8DL_2N_5X8F);
    tmpIF->delay_us(40);

    //Display on cursor off
    Lcd2004_command_write(me,LCD_CMD_DON_CURON);
    tmpIF->delay_us(40);

    //Lcd_Clear();
    //entry mode set
    Lcd2004_command_write(me,LCD_CMD_INCADD);
    tmpIF->delay_us(40);
}
#endif


/*==================================================================================================
*                                        GLOBAL FUNCTIONS
==================================================================================================*/
/*************************** Basic functions **********************************/
void Lcd2004_init(Lcd2004 *me, Lcd2004IF *meIF)
{
    //ASSERT((me!=NULL)&&(meIF!=NULL));
    me->meIF = meIF;
#if LCD_OPERATION_MODE	== LCD_8_BIT_MODE
    Lcd2004_8bits_Mode_Init(me);
#elif LCD_OPERATION_MODE	== LCD_4_BIT_MODE
    Lcd2004_4bits_Mode_Init(me);
#endif
    Lcd2004_Clear(me);
}

void Lcd2004_Clear(Lcd2004 *me)
{
    //ASSERT(me!=NULL);
    
    Lcd2004IF *tmpIF = me->meIF;
    /*write clear command from ic1 to ic0 respectively cursor will stop at pos 0 of ic0 */
    Lcd2004_command_write(me,LCD_CMD_DIS_CLEAR);
    /*delay more than 1.52ms for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_ms(LCD_CLEAR_CMD_DELAY);
    //Lcd2004_Set_Cursor(me,0,0);
}

void Lcd2004_Return_Home(Lcd2004 *me)
{
    //ASSERT(me!=NULL);
    
    Lcd2004IF *tmpIF = me->meIF;
    Lcd2004_command_write(me,LCD_CMD_DIS_RETURN_HOME);
    tmpIF->delay_ms(LCD_RETURN_HOME_CMD_DELAY);
}

void Lcd2004_Set_Cursor(Lcd2004 *me, uint8_t x, uint8_t y)
{
    //ASSERT(me!=NULL);
    
    Lcd2004IF *tmpIF = me->meIF;
    /*check input position valid*/
    if((y > LCD_CHARACTER_LINES - 1) || (x > MAX_CHARACTER_OF_LINE - 1))
    {
        return;
    }

    switch(y & 0x3)
    {
        case 0:
            Lcd2004_command_write(me, 0x80 | x);
            break;
        case 1:
            Lcd2004_command_write(me, 0x80 | (0x40 + x));
            break;
        case 2:
            Lcd2004_command_write(me, 0x80 | (0x14 + x));
            break;
        case 3:
            Lcd2004_command_write(me, 0x80 | (0x54 + x));
            break;
        default:
            break;
    }
    /*delay more than 37us for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}


void Lcd2004_Put_Char(Lcd2004 *me, uint8_t data)
{
/*
 * This function sends a character to the LCD
 * Here we are using 4 bits parallel data transmission
 * */
    //ASSERT(me!=NULL);
    Lcd2004IF *tmpIF = me->meIF;
    /*RS = 1, for LCD user data*/
    tmpIF->data_mode();
    Lcd2004_data_write(me, data);
    tmpIF->delay_us(LCD_SET_RAM_CMD_DELAY);
}


void Lcd2004_Put_String(Lcd2004 *me, uint8_t x, uint8_t y, uint8_t *pdata, uint16_t len)
{
    /* if Line/col equal by 0xff print at current posistion*/
    //ASSERT(me!=NULL);
    uint16_t i=0;
    /* Check input validation */
    if((y > LCD_CHARACTER_LINES - 1) || (x > MAX_CHARACTER_OF_LINE - 1))
    {
        return;
    }
    if((y != LCD_CURRENT_POSITION) && (x != LCD_CURRENT_POSITION))
    {
        Lcd2004_Set_Cursor(me, x, y);
    }
    for (i=0 ; i<len ; i++)
    {
        Lcd2004_Put_Char(me, (uint8_t)pdata[i]);
    }
}

void Lcd2004_Cursor_On(Lcd2004 *me)
{
    //ASSERT(me!=NULL);
    
    Lcd2004IF *tmpIF = me->meIF;
    Lcd2004_command_write(me, LCD_CMD_DON_CURON);
    /*delay more than 37us for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}
void Lcd2004_Cursor_Off(Lcd2004 *me)
{
    //ASSERT(me!=NULL);
    
    Lcd2004IF *tmpIF = me->meIF;
    Lcd2004_command_write(me, LCD_CMD_DON_CUROFF);
    /*delay more than 37us for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}
void Lcd2004_Cursor_Blink(Lcd2004 *me)
{
    //ASSERT(me!=NULL);
    
    Lcd2004IF *tmpIF = me->meIF;
    Lcd2004_command_write(me, LCD_CMD_DON_CURBLINK);
    /*delay more than 37us for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}

void Lcd2004_Move_Cursor_Right(Lcd2004 *me)
{
    //ASSERT(me!=NULL);
    
    Lcd2004IF *tmpIF = me->meIF;

	/*shift cursor position to the right*/
	Lcd2004_command_write(me, LCD_CURSOR_MOVE|LCD_SHIFT_RIGHT);
	tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}

void Lcd2004_Move_Cursor_Left(Lcd2004 *me)
{
    //ASSERT(me!=NULL);
    
    Lcd2004IF *tmpIF = me->meIF;

	/*shift cursor position to the left*/
	Lcd2004_command_write(me, LCD_CURSOR_MOVE|LCD_SHIFT_LEFT);
	tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}

void Lcd2004_Set_Contrast(Lcd2004 *me, uint32_t contrastValue)
{
	//ASSERT(me!=NULL);
	Lcd2004IF *tmpIF = me->meIF;
	tmpIF->contrast(contrastValue);
}
