/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "lcd1602.h"
#include "standard.h"
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
static void Lcd1602_4bits_Mode_Init(Lcd1602 *me);
static void Lcd1602_data_write(Lcd1602 *me, uint8_t data);
static void Lcd1602_Display_Move_Right(Lcd1602 *me);
static void Lcd1602_Display_Move_Left(Lcd1602 *me);
/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/
static void Lcd1602_data_write(Lcd1602 *me, uint8_t data)
{
    ASSERT(me!=NULL);
    Lcd1602IF *tmpIF = me->meIF;
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

static void Lcd1602_command_write(Lcd1602 *me, uint8_t cmd)
{
    ASSERT(me!=NULL);
    Lcd1602IF *tmpIF = me->meIF;
    /*RS = 0, for LCD command*/
    tmpIF->instruction_mode();
    Lcd1602_data_write(me, cmd);

}

#if LCD_OPERATION_MODE	== LCD_4_BIT_MODE
static void Lcd1602_4bits_Mode_Init(Lcd1602 *me)
{
    ASSERT(me!=NULL);
    Lcd1602IF *tmpIF = me->meIF;
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
    Lcd1602_command_write(me,LCD_CMD_4DL_2N_5X8F);
    tmpIF->delay_us(40);

    //Display on cursor on
    Lcd1602_command_write(me,LCD_CMD_DON_CURON);
    tmpIF->delay_us(40);

    //Lcd1602_Clear(me);
    //entry mode set
    Lcd1602_command_write(me,LCD_CMD_INCADD);
    tmpIF->delay_us(40);
}
#elif LCD_OPERATION_MODE	== LCD_8_BIT_MODE
static void Lcd1602_8bits_Mode_Init(Lcd1602 *me)
{
    ASSERT(me!=NULL);
    Lcd1602IF *tmpIF = me->meIF;
    //1. Do the lcd initialization
    tmpIF->delay_us(40);
    /*RS = 0, for LCD command*/
    tmpIF->instruction_mode();

    Lcd1602_command_write(me,0x30U);
    tmpIF->delay_us(5);
    Lcd1602_command_write(me,0x30U);
    tmpIF->delay_us(200);

    Lcd1602_command_write(me,0x30U);

    //function set command
    Lcd1602_command_write(me,LCD_CMD_8DL_2N_5X8F);
    tmpIF->delay_us(40);

    //Display on cursor off
    Lcd1602_command_write(me,LCD_CMD_DON_CURON);
    tmpIF->delay_us(40);

    //Lcd_Clear();
    //entry mode set
    Lcd1602_command_write(me,LCD_CMD_INCADD);
    tmpIF->delay_us(40);
}
#endif


/*==================================================================================================
*                                        GLOBAL FUNCTIONS
==================================================================================================*/
/*************************** Basic functions **********************************/
void Lcd1602_init(Lcd1602 *me, Lcd1602IF *meIF)
{
    ASSERT((me!=NULL)&&(meIF!=NULL));
    me->meIF = meIF;
#if LCD_OPERATION_MODE	== LCD_8_BIT_MODE
    Lcd1602_8bits_Mode_Init(me);
#elif LCD_OPERATION_MODE	== LCD_4_BIT_MODE
    Lcd1602_4bits_Mode_Init(me);
#endif
    Lcd1602_Clear(me);
}

void Lcd1602_Clear(Lcd1602 *me)
{
    ASSERT(me!=NULL);
    
    Lcd1602IF *tmpIF = me->meIF;
    /*write clear command from ic1 to ic0 respectively cursor will stop at pos 0 of ic0 */
    Lcd1602_command_write(me,LCD_CMD_DIS_CLEAR);
    /*delay more than 1.52ms for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_ms(LCD_CLEAR_CMD_DELAY);
    //Lcd1602_Set_Cursor(me,0,0);
}

void Lcd1602_Return_Home(Lcd1602 *me)
{
    ASSERT(me!=NULL);
    
    Lcd1602IF *tmpIF = me->meIF;
    Lcd1602_command_write(me,LCD_CMD_DIS_RETURN_HOME);
    tmpIF->delay_ms(LCD_RETURN_HOME_CMD_DELAY);
}

void Lcd1602_Set_Cursor(Lcd1602 *me, uint8_t x, uint8_t y)
{
    ASSERT(me!=NULL);
    
    Lcd1602IF *tmpIF = me->meIF;
    /*check input position valid*/
    if((y > LCD_CHARACTER_LINES - 1) || (x > MAX_CHARACTER_OF_LINE - 1))
    {
        return;
    }

    switch(y & 0x1)
    {
        case 0:
            Lcd1602_command_write(me, x | 0x80);
            break;
        case 1:
            Lcd1602_command_write(me, x | 0xC0);
            break;
        default:
            break;
    }
    /*delay more than 37us for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}


void Lcd1602_Put_Char(Lcd1602 *me, uint8_t data)
{
/*
 * This function sends a character to the LCD
 * Here we are using 4 bits parallel data transmission
 * */
    ASSERT(me!=NULL);
    Lcd1602IF *tmpIF = me->meIF;
    /*RS = 1, for LCD user data*/
    tmpIF->data_mode();
    Lcd1602_data_write(me, data);
    tmpIF->delay_us(LCD_SET_RAM_CMD_DELAY);
}


void Lcd1602_Put_String(Lcd1602 *me, uint8_t x, uint8_t y, uint8_t *pdata, uint16_t len)
{
    /* if Line/col equal by 0xff print at current posistion*/
    ASSERT(me!=NULL);
    uint16_t i=0;
    /* Check input validation */
    if((y > LCD_CHARACTER_LINES - 1) || (x > MAX_CHARACTER_OF_LINE - 1))
    {
        return;
    }
    if((y != LCD_CURRENT_POSITION) && (x != LCD_CURRENT_POSITION))
    {
        Lcd1602_Set_Cursor(me, x, y);
    }
    for (i=0 ; i<len ; i++)
    {
        Lcd1602_Put_Char(me, (uint8_t)pdata[i]);
    }
}

void Lcd1602_Cursor_On(Lcd1602 *me)
{
    ASSERT(me!=NULL);
    
    Lcd1602IF *tmpIF = me->meIF;
    Lcd1602_command_write(me, LCD_CMD_DON_CURON);
    /*delay more than 37us for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}
void Lcd1602_Cursor_Off(Lcd1602 *me)
{
    ASSERT(me!=NULL);
    
    Lcd1602IF *tmpIF = me->meIF;
    Lcd1602_command_write(me, LCD_CMD_DON_CUROFF);
    /*delay more than 37us for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}
void Lcd1602_Cursor_Blink(Lcd1602 *me)
{
    ASSERT(me!=NULL);
    
    Lcd1602IF *tmpIF = me->meIF;
    Lcd1602_command_write(me, LCD_CMD_DON_CURBLINK);
    /*delay more than 37us for command process
    * Check page num24 of datasheet
    */
    tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}

void Lcd1602_Move_Cursor_Right(Lcd1602 *me)
{
    ASSERT(me!=NULL);
    
    Lcd1602IF *tmpIF = me->meIF;

	/*shift cursor position to the right*/
	Lcd1602_command_write(me, LCD_CURSOR_MOVE|LCD_SHIFT_RIGHT);
	tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}

void Lcd1602_Move_Cursor_Left(Lcd1602 *me)
{
    ASSERT(me!=NULL);
    
    Lcd1602IF *tmpIF = me->meIF;

	/*shift cursor position to the left*/
	Lcd1602_command_write(me, LCD_CURSOR_MOVE|LCD_SHIFT_LEFT);
	tmpIF->delay_us(LCD_CURSOR_CMD_DELAY);
}

void Lcd1602_Set_Contrast(Lcd1602 *me, uint32_t contrastValue)
{
	ASSERT(me!=NULL);
	Lcd1602IF *tmpIF = me->meIF;
	tmpIF->contrast(contrastValue);
}
