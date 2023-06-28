

#ifndef DS1307_H_
#define DS1307_H_

#define __RX_MCU__
#define __ISL12008__
//#define __ISL12008__

#if defined(__ST_MCU__) 
#include "stm32f407xx.h"
#elif defined(__RX_MCU__)
#include "r_smc_entry.h"
#endif


#if defined(__ST_MCU__)

/*Application configurable items */
#define RTC_I2C  			I2C1
#define RTC_I2C_GPIO_PORT    GPIOB
#define RTC_I2C_SDA_PIN 		GPIO_PIN_NO_7
#define RTC_I2C_SCL_PIN 		GPIO_PIN_NO_6
#define RTC_I2C_SPEED 		I2C_SCL_SPEED_SM
#define RTC_I2C_PUPD			GPIO_PIN_PU

#endif


/*Register addresses */
                                                    /*           7       6       5       4       3       2       1       0  */
/*  */                                                                                                                
#define RTC_ADDR_SEC 		            0x00    /*SC:       ST    SC22    SC21    SC20    SC13    SC12    SC11    SC10  */
#define RTC_ADDR_MIN 		            0x01    /*MN:       OF    MN22    MN21    MN20    MN13    MN12    MN11    MN10  */
#define RTC_ADDR_HRS	                0x02    /*HR:      CEB      CB    HR21    HR20    HR13    HR12    HR11    HR10  */
#define RTC_ADDR_DAY		            0x03    /*DW:        0       0       0       0       0    DW12    DW11    DW10  */
#define RTC_ADDR_DATE		            0x04    /*DT:        0       0    DT21    DT20    DT13    DT12    DT11    DT10  */
#define RTC_ADDR_MONTH		            0x05    /*MO:        0       0       0    MO20    MO13    MO12    MO11    MO10  */
#define RTC_ADDR_YEAR		            0x06    /*YR:     YR23    YR22    YR21    YR20    YR13    YR12    YR11    YR10  */
                                                                                                                      
#define RTC_ADDR_DIGITAL_TRIM          0x07    /*DTR:     OUT      FT    DTR5    DTR4    DTR3    DTR2    DTR1    DTR0  */
#define RTC_ADDR_INTERRUPT_CTRL        0x08    /*INT:       0    ALME  LPMODE       0       0       0       0       0  */
#define RTC_ADDR_ANALOG_TRIM           0x0A    /*ATR:  BMATR1  BMATR0    ATR5    ATR4    ATR3    ATR2    ATR1    ATR0  */
#define RTC_ADDR_STATUS                0x0B    /*SR:     ARST   XSTOP  RESEAL       0       0     ALM     BAT    RTCF  */

#if defined(__DS1307__)
#define TIME_FORMAT_12HRS_AM 	0
#define TIME_FORMAT_12HRS_PM 	1
#define TIME_FORMAT_24HRS 		2
#endif

#if defined(__ISL12008__)
#define RTC_1900s_CENTURY       0
#define RTC_2000s_CENTURY       1
#endif

/* RTC module state*/
#define RTC_RUNNING_STATE       0
#define RTC_START_COUNT_ERROR   1
#define RTC_CRYSTAL_OSC_ERROR   2

#define RTC_PAUSE_STATE         RTC_START_COUNT_ERROR


#define RTC_I2C_ADDRESS      0x68


#define SUNDAY  	1;
#define MONDAY  	2;
#define TUESDAY  	3;
#define WEDNESDAY   4;
#define THURSDAY  	5;
#define FRIDAY  	6;
#define SATURDAY  	7;



typedef struct
{
	uint8_t date;
	uint8_t month;
	uint8_t year;
	uint8_t day;
#if defined(__ISL12008__)
	uint8_t century;
#endif
}RTC_date_t;


typedef struct
{
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
#if defined(__DS1307__)
	uint8_t time_format;
#endif
}RTC_time_t;



//Function prototypes

/*  @Brief: Initialize rtc function and start running counting 
*   @Return: 0: RTC_RUNNING_STATE     ~ RTC is in running state                 
*            1: RTC_START_COUNT_ERROR ~ RTC is in stopping state
*            2: RTC_CRYSTAL_OSC_ERROR ~ RTC has no crystal osc connection
*           
*/
uint8_t rtc_init(void);

/* 
*   @Brief: Check RTC module is running or not
*   @Return: 0: RTC_RUNNING_STATE     ~ RTC is in running state                 
*            1: RTC_PAUSE_STATE       ~ RTC is in stopping state
*            
*/
uint8_t rtc_isrunning(void);


/*  @Brief: Start and stop RTC function
*   @Note:  RTC Counter already run at initilize time
*   @Return: 0: RTC_RUNNING_STATE     ~ RTC is in running state                 
*            1: RTC_PAUSE_STATE       ~ RTC is in stopping state       
*/
uint8_t rtc_start(void);
uint8_t rtc_stop(void);


/*
*   @Brief: enable/disable  Frequency test pinout
*
*   @Note: The gating time should be set long enough to ensure accuracy
*          to at least 1ppm. To calculate the ppm on the measured 512Hz, 
*          simply following:
*          ppm = (FT/512 - 1)* 10^6 
*
*/
void rtc_enable_FTout(void);
void rtc_disable_FTout(void);

/* @Brief: Set and Get current Time
*           
*/
void rtc_set_current_time(RTC_time_t *rtc_time);
void rtc_get_current_time(RTC_time_t *rtc_time);

/* @Brief: Set and Get current Date
*           
*/
void rtc_set_current_date(RTC_date_t *rtc_date);
void rtc_get_current_date(RTC_date_t *rtc_date);

#if defined(__ISL12008__)
/* 
*   @Brief: LPMODE will saving 600nA at VDD = 5V of switching voltage source block
*
*   in normal mode Vbat switching conditions is:
*           - VDD < VBAT - VBATHYS 
*           - VDD < VTRIP 
*   in LPMODE : 
*           - VDD < VTRIP
*
*   @Note : it is not recommended to use Low Power Mode in a system with 
*             VDD = 3.3V ±10%, VBAT  3.0V, and when there is a finite I-R voltage
*             drop in the VDD line 
*/
/* These functions only support specific to renesas isl12008 ic */
void rtc_enable_lowpowermode(void);
void rtc_disable_lowpowermode(void);

/* @Brief: Digital Trimming to adjust the average number of counts per second and average the
*          ppm error to achieve better accuracy
*  @input: calValue  ( 5 bits sign number to adjustment )
*          calValue < 0  ~  adjustment value = -2,0345ppm * calValue
*          calValue > 0  ~  adjustment value =  4.0690ppm * calValue
*          -31 <= calValue < = 31
*/
void rtc_set_DigitalCalibration(char calValue);
char rtc_get_DigitalCalibration(void);

/* @Brief: the Date return only be 2 LSB digits of the year value and
*         2 MSB digits will depend on century bit that be defined 
*         these functions will make CB century bit toggle when the years register overflows from 99 to 00
*   
*         Eg: defined 0 : it is known as 1900 century 
*                     1 : it is known as 2000 century 
*
*/
void rtc_enable_century_toggle(void);
void rtc_disable_century_toggle(void);

/* @Brief: enables/disables the device enter into the InterSeal™
*          Battery Saver mode after manufacture testing for board functionality
*  @Input: 0 : enable the battery function during normal operation or full functional testing
*          1 : enable the InterSeal™ Battery Saver mode and prevents battery current drain before it is first used
*
*
*/
void rtc_set_InterSeal(uint8_t reseal);
uint8_t rtc_get_InterSeal(void);
#endif

#endif /* RTC_H_ */
