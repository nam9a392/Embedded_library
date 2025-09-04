
#include<stdint.h>
#include<string.h>

#include "rtc_isl12008.h"


#if defined(__ST_MCU__)
#define RTC_I2C_MasterSendData(address,buf,len,timeout)       I2C_MasterSendData(&g_isl12008I2cHandle, buf, len, address, timeout)
#define RTC_I2C_MasterReceiveData(address,buf,len,timeout)    I2C_MasterReceiveData(&g_isl12008I2cHandle, buf, len, address, timeout)
#elif defined(__RX_MCU__)
#define RTC_I2C_MasterSendData(address,buf,len,timeout)       my_SCI2_i2c_Master_Send(address , buf, len)
#define RTC_I2C_MasterReceiveData(address,buf,len,timeout)    my_SCI2_i2c_Master_Receive(address, buf, len)
#endif

static rtcIF_t *g_rtcIF;
/********************************** Local function prototype **************************************/
static void rtc_i2c_init(void);
static void rtc_i2c_pin_config(void);
static void rtc_i2c_config(void);
static uint8_t rtc_read(uint8_t reg_addr);
static void rtc_write(uint8_t value,uint8_t reg_addr);
static uint8_t bcd_to_binary(uint8_t value);
static uint8_t binary_to_bcd(uint8_t value);

/********************************** Local function ************************************************/

/* i2C init for ST platfrom*/
static void rtc_i2c_init(void)
{
#if defined(__ST_MCU__)
    //1. init the i2c pins
	rtc_i2c_pin_config();

	//2. initialize the i2c peripheral
	rtc_i2c_config();

	//3. Enable the I2C peripheral
	I2C_PeripheralControl(RTC_I2C, ENABLE);
#elif defined(__RX_MCU__)
    /* using I2C comunication from sci1 RX mcu */
	//R_Config_RIIC0_Start();
#endif
}


#if defined(__ST_MCU__)
I2C_Handle_t g_rtcI2cHandle;
static void rtc_i2c_pin_config(void)
{
	GPIO_Handle_t i2c_sda,i2c_scl;

	memset(&i2c_sda,0,sizeof(i2c_sda));
	memset(&i2c_scl,0,sizeof(i2c_scl));

	/*
	 * I2C1_SCL ==> PB6
	 * I2C1_SDA ==> PB7
	 */

	i2c_sda.pGPIOx = RTC_I2C_GPIO_PORT;
	i2c_sda.GPIO_PinConfig.GPIO_PinAltFunMode = 4;
	i2c_sda.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	i2c_sda.GPIO_PinConfig.GPIO_PinNumber = RTC_I2C_SDA_PIN;
	i2c_sda.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD;
	i2c_sda.GPIO_PinConfig.GPIO_PinPuPdControl = RTC_I2C_PUPD;
	i2c_sda.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

	GPIO_Init(&i2c_sda);


	i2c_scl.pGPIOx = RTC_I2C_GPIO_PORT;
	i2c_scl.GPIO_PinConfig.GPIO_PinAltFunMode = 4;
	i2c_scl.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	i2c_scl.GPIO_PinConfig.GPIO_PinNumber = RTC_I2C_SCL_PIN;
	i2c_scl.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD;
	i2c_scl.GPIO_PinConfig.GPIO_PinPuPdControl = RTC_I2C_PUPD;
	i2c_scl.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

	GPIO_Init(&i2c_scl);
}

static void rtc_i2c_config(void)
{

	g_rtcI2cHandle.pI2Cx = RTC_I2C;
	g_rtcI2cHandle.I2C_Config.I2C_AckControl = I2C_ACK_ENABLE;
	g_rtcI2cHandle.I2C_Config.I2C_SCLSpeed = RTC_I2C_SPEED;
	I2C_Init(&g_rtcI2cHandle);

}
#endif

/*************************************************************************************/
static void rtc_write(uint8_t value,uint8_t reg_addr)
{
	uint8_t tx[2];
	tx[0] = reg_addr;
	tx[1] = value;
//	RTC_I2C_MasterSendData(RTC_I2C_ADDRESS, tx, 2,  0);
	g_rtcIF->i2c_write(RTC_I2C_ADDRESS, tx, 2,  0);
}

static uint8_t rtc_read(uint8_t reg_addr)
{
	uint8_t data;
//    RTC_I2C_MasterSendData(RTC_I2C_ADDRESS,  &reg_addr, 1, 0);
//    RTC_I2C_MasterReceiveData(RTC_I2C_ADDRESS, &data, 1, 0);
	g_rtcIF->i2c_write(RTC_I2C_ADDRESS,  &reg_addr, 1, 0);
	g_rtcIF->i2c_read(RTC_I2C_ADDRESS, &data, 1, 0);

    return data;
}

static uint8_t binary_to_bcd(uint8_t value)
{
	uint8_t m , n;
	uint8_t bcd;

	bcd = value;
	if(value >= 10)
	{
		m = value /10;
		n = value % 10;
		bcd = (m << 4) | n ;
	}

	return bcd;
}

static uint8_t bcd_to_binary(uint8_t value)
{
	uint8_t m , n;
	m = (uint8_t) ((value >> 4 ) * 10);
	n =  value & (uint8_t)0x0F;
	return (m+n);
}



/********************************** Global API function ************************************************/
uint8_t rtc_init(rtcIF_t *meIF)
{
    uint8_t reg_val = 0;
    g_rtcIF = meIF;
    /* initial i2c peripheral of mcu that control rtc module */
    rtc_i2c_init();
	
    /*****************************************/
#if defined(__ISL12008__) 
    /* check if that module stil be real time ~ start from batery backup power*/
    uint8_t rtcf, sr_reg, bat;
    sr_reg = rtc_read(RTC_ADDR_STATUS);
    rtcf =  sr_reg & 0x1;
    bat = (sr_reg >> 1) & 0x1;

    if(0U == rtcf) // This rtcf bit is set to a “1” after a total power failure
    {
        /* check crystal oscillation connecting status on X1 pin*/
        reg_val = rtc_read(RTC_ADDR_MIN);
        rtc_write(reg_val & ~(1 << 7),RTC_ADDR_MIN); /* Clear OF bit of MN register to check osc status*/
        reg_val = rtc_read(RTC_ADDR_MIN);
        if((reg_val >> 7) & 0x1)
        {
            // there is no crystal osc
            return RTC_CRYSTAL_OSC_ERROR;
        }
        /*clear battery bit */
        rtc_write(reg_val & ~(1 << 1),RTC_ADDR_STATUS);
        /*disable ReSeal function to enable the battery function during normal operation*/
        rtc_set_InterSeal(0);
        /*Enable century bit*/
        rtc_enable_century_toggle();
    }else
#endif
    {
        // Make clock halt = 0;
        rtc_start();
    }
    /* check if counting start correctly*/
    return rtc_isrunning() ;
}


uint8_t rtc_isrunning(void)
{
    //Read back clock halt bit
    uint8_t ret_state = RTC_PAUSE_STATE; 
	uint8_t clock_state = rtc_read(RTC_ADDR_SEC);
    ret_state = ((clock_state >> 7 ) & 0x1) ? RTC_PAUSE_STATE : RTC_RUNNING_STATE;
    
    return ret_state;
}

uint8_t rtc_start(void)
{
    // Make clock halt = 0;
    uint8_t clock_state = rtc_read(RTC_ADDR_SEC);
    /* clear ST bit of SC reg to active counting in RTC register*/
    clock_state = clock_state & ~(1 << 7);
	rtc_write(clock_state,RTC_ADDR_SEC);
    
    return rtc_isrunning();
}

uint8_t rtc_stop(void)
{
    // Make clock halt = 1;
    uint8_t clock_state = rtc_read(RTC_ADDR_SEC);
    /* Set ST bit of SC reg to stop counting in RTC register*/
    clock_state = clock_state | (1 << 7);
	rtc_write(clock_state,RTC_ADDR_SEC);
    
    return rtc_isrunning();
}

void rtc_enable_FTout(void)
{
    uint8_t dtr_reg;
    dtr_reg = rtc_read(RTC_ADDR_DIGITAL_TRIM);
    /* Set FT/OUT bit of INT reg to active 512HZ FREQUENCY OUTPUT*/
    dtr_reg = dtr_reg | (0x3 << 6);
    rtc_write(dtr_reg,RTC_ADDR_DIGITAL_TRIM);
}

void rtc_disable_FTout(void)
{
    uint8_t dtr_reg;
    dtr_reg = rtc_read(RTC_ADDR_DIGITAL_TRIM);
    /* Clear FT bit of INT reg to de-active 512HZ FREQUENCY OUTPUT*/
    dtr_reg = dtr_reg & ~(0x1 << 6);
    rtc_write(dtr_reg,RTC_ADDR_DIGITAL_TRIM);
}

void rtc_set_current_time(RTC_time_t *rtc_time)
{
	uint8_t seconds, minutes, hrs;
	seconds = binary_to_bcd(rtc_time->seconds);
	seconds &= ~( 1 << 7);
	rtc_write(seconds, RTC_ADDR_SEC);

    minutes = binary_to_bcd(rtc_time->minutes);
    minutes &= ~( 1 << 7);
	rtc_write(minutes, RTC_ADDR_MIN);

    hrs = rtc_read(RTC_ADDR_HRS);
	hrs = binary_to_bcd(rtc_time->hours);

    /*************************************/
    /* isl12008 has only support 24hr format*/
#if defined(__DS1307__)
	if(rtc_time->time_format == TIME_FORMAT_24HRS){
		hrs &= ~(1 << 6);
	}else{
		hrs |= (1 << 6);
		hrs = (rtc_time->time_format  == TIME_FORMAT_12HRS_PM) ? hrs | ( 1 << 5) :  hrs & ~( 1 << 5) ;
	}
#elif defined(__ISL12008__)
    /* keep CEB, CB its current century bit */
    hrs = hrs | (rtc_read(RTC_ADDR_HRS) & (0x3 << 6));
#endif
    /*****************************************/
	rtc_write(hrs,RTC_ADDR_HRS);

}

void rtc_set_current_date(RTC_date_t *rtc_date)
{
    uint8_t hrs;
	rtc_write(binary_to_bcd(rtc_date->date),RTC_ADDR_DATE);

	rtc_write(binary_to_bcd(rtc_date->month),RTC_ADDR_MONTH);

	rtc_write(binary_to_bcd(rtc_date->year),RTC_ADDR_YEAR);

	rtc_write(binary_to_bcd(rtc_date->day),RTC_ADDR_DAY);

#if defined(__ISL12008__)
    /* set CEB, CB its current century bit */
    hrs = rtc_read(RTC_ADDR_HRS);
    hrs = (hrs & ~(0x1 << 6)) | (rtc_date->century << 6);
    rtc_write(hrs,RTC_ADDR_HRS);
#endif
}

void rtc_get_current_time(RTC_time_t *rtc_time)
{

	uint8_t seconds, minutes, hrs;

	seconds = rtc_read(RTC_ADDR_SEC);
	seconds &= ~( 1 << 7);
	rtc_time->seconds = bcd_to_binary(seconds);
    
    minutes = rtc_read(RTC_ADDR_MIN);
    minutes &= ~( 1 << 7);
	rtc_time->minutes = bcd_to_binary(minutes);

	hrs = rtc_read(RTC_ADDR_HRS);
    /* isl12008 has only support 24hr format*/
#if defined(__DS1307__)
    /***************************/
	if(hrs & ( 1 << 6)){
		//12 hr format
		rtc_time->time_format =  !((hrs & ( 1 << 5)) == 0) ;
		hrs &= ~(0x3 << 5);//Clear 6 and 5
	}else{
		//24 hr format
		rtc_time->time_format = TIME_FORMAT_24HRS;
	}
    /****************************/
#elif defined(__ISL12008__)
    /* remove century bit before converting */
    hrs &= ~(0x3 << 6);
#endif
	rtc_time->hours = bcd_to_binary(hrs);
}

void rtc_get_current_date(RTC_date_t *rtc_date)
{
	rtc_date->day =  bcd_to_binary(rtc_read(RTC_ADDR_DAY));
	rtc_date->date = bcd_to_binary(rtc_read(RTC_ADDR_DATE));
	rtc_date->month = bcd_to_binary(rtc_read(RTC_ADDR_MONTH));
	rtc_date->year = bcd_to_binary(rtc_read(RTC_ADDR_YEAR));
#if defined(__ISL12008__)
    rtc_date->century = (rtc_read(RTC_ADDR_HRS) >> 6) & 0x1;
#endif
}

/* These functions only support specify to renesas isl12008 ic */
#if defined(__ISL12008__)

void rtc_enable_century_toggle(void)
{
    /*Set century bit*/
    uint8_t reg_val = rtc_read(RTC_ADDR_HRS);
    rtc_write(reg_val | (1 << 7),RTC_ADDR_HRS);
}

void rtc_disable_century_toggle(void)
{
    /*Clear century bit*/
    uint8_t reg_val = rtc_read(RTC_ADDR_HRS);
    rtc_write(reg_val & ~(1 << 7),RTC_ADDR_HRS);
}

void rtc_enable_lowpowermode(void)
{
    uint8_t int_reg;
    int_reg = rtc_read(RTC_ADDR_INTERRUPT_CTRL);
    /* Set LPMODE bit of INT reg to active low power mode*/
    int_reg = int_reg | (1 << 5);
    rtc_write(int_reg,RTC_ADDR_INTERRUPT_CTRL);
}

void rtc_disable_lowpowermode(void)
{
    uint8_t int_reg;
    int_reg = rtc_read(RTC_ADDR_INTERRUPT_CTRL);
    /* Clear LPMODE bit of INT reg to disactive low power mode*/
    int_reg = int_reg & ~(1 << 5);
    rtc_write(int_reg,RTC_ADDR_INTERRUPT_CTRL);
}

void rtc_set_DigitalCalibration(char calValue)
{
    uint8_t dtr_reg;
    uint8_t calReg = abs(calValue) & 0x1f;
    if (calValue >= 0) 
    {
        calReg |= 0x20; // Sign bit is positive to speed up the clock
    }
    dtr_reg = rtc_read(RTC_ADDR_DIGITAL_TRIM);
    dtr_reg = (dtr_reg & ~0x3F) | calReg;
    rtc_write(dtr_reg,RTC_ADDR_DIGITAL_TRIM);
}

char rtc_get_DigitalCalibration(void)
{
    uint8_t dtr_reg;
    dtr_reg = rtc_read(RTC_ADDR_DIGITAL_TRIM);
    char out = dtr_reg & 0x1F;
    if(!(dtr_reg & 0x20)) 
    {
        out = -out; //S bit clear means a negative value
    }
    return out;
}

void rtc_set_InterSeal(uint8_t reseal)
{
    uint8_t sr;
    sr = rtc_read(RTC_ADDR_STATUS);
    if(reseal == 0)
    {
        /* clear RESEAL bit */
        sr = sr & ~(0x1 << 5);
    }else{
        /* set RESEAL bit */
        sr = sr | (0x1 << 5);
    }
    rtc_write(sr,RTC_ADDR_STATUS);
}

uint8_t rtc_get_InterSeal(void)
{
    return (rtc_read(RTC_ADDR_STATUS) >> 5)& 0x1;
}

/* Analog adjustment */
/***************** @TODO *****************/
void rtc_set_AnalogCalibration(uint8_t calValue)
{
    
}

void rtc_get_AnalogCalibration(uint8_t calValue)
{
    
}
/*****************************************/
#endif

