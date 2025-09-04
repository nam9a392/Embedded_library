/*
 * lcd40x4.h
 *
 *  Created on: Sep 5, 2023
 *      Author: ADMIN
 */

#ifndef DRIVER_LCDCHARACTER40X4_LCD40X4_H_
#define DRIVER_LCDCHARACTER40X4_LCD40X4_H_

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
/**************************** Define display constants ****************************/
#define LCD_CLEAR_CMD_DELAY             		2    //ms
#define LCD_RETURN_HOME_CMD_DELAY       		2    //ms
#define LCD_CURSOR_CMD_DELAY            		40   //us
#define LCD_SET_RAM_CMD_DELAY           		40   //us

#define LCD_8_BIT_MODE							0
#define LCD_4_BIT_MODE							1
#define LCD_OPERATION_MODE						LCD_4_BIT_MODE
#define LCD_IC_NUMBER		                    2
#define LCD_CHARACTER_LINES                     4
#define MAX_CHARACTER_OF_LINE                   40
#define MAX_CHARACTER_OF_DISPLAY                40

#define LCD_CHIP_0								0
#define LCD_CHIP_1								2

#define LCD_CURRENT_POSITION                    0xff

#define LCD_PUT_CHAR_CURSOR_MOVE_MODE           0
#define LCD_PUT_CHAR_CURSOR_STATIC_MODE         1
/**************************** Macro display hal function **************************/


/**************************** Define display commands ****************************/
/*Lcd commands  */
/* 4bits, 2lines, 5x8 */
#define LCD_CMD_4DL_2N_5X8F			            0x28
/* function set is 8 bit data length and 2 lines */
#define LCD_CMD_8DL_2N_5X8F			            LCD_FUNCTIONSET | LCD_8BITMODE | LCD_2LINE | LCD_5x8DOTS // 0x38
/* Display on, cursor on, cursor not blink */
#define LCD_CMD_DON_CURON			            LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKOFF // 0x0E
/* Display on, cursor off, cursor not blink */
#define LCD_CMD_DON_CUROFF			            LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF //0x0C
/* Display on, cursor on, cursor blink */
#define LCD_CMD_DON_CURBLINK	                LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKON // 0x0F

/* cursor move direction is increment */
/* Entry mode is display Data RAM pointer incremented after write */
#define LCD_CMD_INCADD				            LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT //0x06
/* cursor move direction is increment */
#define LCD_CMD_DECADD				            LCD_ENTRYMODESET | LCD_ENTRYRIGHT | LCD_ENTRYSHIFTDECREMENT
/* Clear entire display */
#define LCD_CMD_DIS_CLEAR			            LCD_CLEARDISPLAY
/* Return Cursor to home */
#define LCD_CMD_DIS_RETURN_HOME		            LCD_RETURNHOME

#define LCD_DISPLAY_SHIFT                       LCD_CURSORSHIFT | LCD_DISPLAYMOVE //0x18
#define LCD_CURSOR_MOVE                         LCD_CURSORSHIFT | LCD_CURSORMOVE  //0X10
#define LCD_SHIFT_RIGHT                         LCD_MOVERIGHT
#define LCD_SHIFT_LEFT                          LCD_MOVELEFT


// commands
#define LCD_CLEARDISPLAY 						0x01
#define LCD_RETURNHOME 							0x02		// sets DDRAM address 0 and returns display from being shifted to original position.
#define LCD_ENTRYMODESET 						0x04
#define LCD_DISPLAYCONTROL 						0x08
#define LCD_CURSORSHIFT 						0x10
#define LCD_FUNCTIONSET 						0x20
#define LCD_SETCGRAMADDR 						0x40		// Address Character Generator RAM pointer
#define LCD_SETDDRAMADDR 						0x80		// Address Display Data RAM pointer

// flags for display entry mode
#define LCD_ENTRYRIGHT 							0x00
#define LCD_ENTRYLEFT 							0x02
#define LCD_ENTRYSHIFTINCREMENT 				0x01
#define LCD_ENTRYSHIFTDECREMENT 				0x00

// flags for display on/off control
#define LCD_DISPLAYON 							0x04
#define LCD_DISPLAYOFF 							0x00
#define LCD_CURSORON 							0x02	// turn on cursor
#define LCD_CURSOROFF 							0x00	// turn off cursor
#define LCD_BLINKON 							0x01
#define LCD_BLINKOFF 							0x00
#define LCD_CURSORS_MASK 						0xfc

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 						0x08
#define LCD_CURSORMOVE 							0x00
#define LCD_MOVERIGHT 							0x04
#define LCD_MOVELEFT 							0x00

// flags for function set
#define LCD_8BITMODE 							0x10
#define LCD_4BITMODE 							0x00
#define LCD_2LINE 								0x08
#define LCD_1LINE 								0x00
#define LCD_5x10DOTS 							0x04
#define LCD_5x8DOTS 							0x00
#define LCD_Right 								0
#define LCD_Left 								1



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
    void (*instruction_mode)(void);
    void (*data_mode)(void);
    void (*rw)(uint8_t _val);
    void (*enable_1)(void);
    void (*enable_2)(void);
    void (*write_4bit)(uint8_t _val);
    void (*write_8bit)(uint8_t _val);
    void (*delay_us)(uint32_t _us);
    void (*delay_ms)(uint32_t _ms);
    void (*contrast)(uint32_t _per);
}Lcd4004IF;

typedef struct{
    Lcd4004IF *meIF;
    uint8_t	   displaycontrol;
    uint8_t    chip;
}Lcd4004;
/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/*************************** Basic functions **********************************/
void Lcd4004_init(Lcd4004 *me, Lcd4004IF *meIF);

/**
 * @brief  This function uses to clear the LCD
 *
 * @param[in]  None
 *
 * @retval void
 *
 */
void Lcd4004_Clear(Lcd4004 *me);

void Lcd4004_Return_Home(Lcd4004 *me);

/**
 * @brief  This function uses to put character to current position
 *
 * @param[in]  data   :ASCII character
 *
 * @retval void
 *
 */
void Lcd4004_Put_Char(Lcd4004 *me, uint8_t data);

/**
 * @brief  This function uses to put string to (x,y) position
 *
 * @param[in]  x         :x position
 * @param[in]  y         :y position
 * @param[in]  pString   :pString string pointer
 *
 * @retval void
 *
 */
void Lcd4004_Put_String(Lcd4004 *me, uint8_t line, uint8_t offset, uint8_t *pString, uint16_t length);

 /*
 * @brief  This function uses to set the lcd cursor position
 * @param[in]  line    : line number (0 to 1)
 *             offset  : offset position in line number (0 to 15) Assuming a 2 x 16 characters display
 *
 * @retval void
 *
 */
void Lcd4004_Set_Cursor(Lcd4004 *me, uint8_t line, uint8_t offset);
void Lcd4004_Move_Cursor_Right(Lcd4004 *me);
void Lcd4004_Move_Cursor_Left(Lcd4004 *me);

void Lcd4004_Cursor_Effect(Lcd4004 *me, uint8_t CursorEffect);
/**
 * @brief  This function uses to turn on the lcd cursor
 *
 * @param[in]  line    : line number
 *             offset  : offset position in line number
 *
 * @retval void
 *
 */
void Lcd4004_Cursor_On(Lcd4004 *me);
/**
 * @brief  This function uses to turn off the lcd cursor
 *
 * @param[in]  none
 *
 * @retval void
 *
 */
void Lcd4004_Cursor_Off(Lcd4004 *me);

void Lcd4004_Cursor_Blink(Lcd4004 *me);
/*************************** Extended functions **********************************/
/* Contrast control by trimmer 10K not by PWM so that it don't need this func*/
/**
 * @brief  This function uses to set contrast value of LCD character
 *
 * @param[in]  uint8_t contrastValue 0 -> 100
 *
 * @retval None
 *
 */

void Lcd4004_Set_Contrast(Lcd4004 *me,uint32_t contrastValue);

#endif /* DRIVER_LCDCHARACTER40X4_LCD40X4_H_ */
