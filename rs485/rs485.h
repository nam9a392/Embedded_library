/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "rs_packet.h"
#include "ring_buffer.h"

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define RS485_LIB_VERSION        			(("0.1.0"))

#define RS485_MAX_DATA_LENGTH         		256
#define RS485_MAX_CHANNEL_NUMBER			32
#define RS485_INVALID_CHANNEL_ID			RS485_MAX_CHANNEL_NUMBER

#define MAX_RETRY_NUMBER					3
#define RS485_TICK_US						200  	// duration per tick in us
#define RS485_T35_DURATION_US				1000	// duration t35 in us

/*==================================================================================================
*                                              ENUMS
==================================================================================================*/
typedef enum{
	RS485_CHANNEL_INIT_STATE,
	RS485_CHANNEL_OFFLINE_STATE,
    RS485_CHANNEL_ONLINE_STATE,
	RS485_CHANNEL_NONE_STATE,
}rs485_channel_state_e;

typedef enum{
	MASTER_MODE,
	SLAVE_MODE
}rs485_bus_mode_e;

typedef enum{
	BUS_MSG_COUNT,
	BUS_ERR_COUNT,
	BUS_OVERRUN_COUNT,
}rs485_diagnostic_e;


/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

typedef Packet_t rs485_msg;
typedef uint32_t rs485timer_t;
typedef struct rs485_state_machine_data 	rs485_state_machine_data_t;
typedef struct rs485 						rs485_t;

typedef struct{
    void (*txMode)(void);
    void (*rxMode)(void);
    void (*uart_rx)(uint8_t *pdata,uint16_t len);
    void (*uart_tx)(uint8_t *pdata,uint16_t len);
}rs485IF_t;

struct rs485_state_machine_data{
	rs485_t			    	*common;
    uint8_t                	cur_state;
    uint8_t                	next_state;
    rs485timer_t            timer;
    uint8_t					cur_poll_state;
    uint8_t					cur_sellect_state;
	volatile rs485timer_t	_timeout_timer;
	volatile rs485timer_t	_t35_timer; 			// frame silent interval 3.5 character
	volatile rs485timer_t	_t15_timer;
    uint8_t		          	internal_event;
};

typedef struct{
    volatile uint32_t   _bus_msg_count;			// CPT1
    volatile uint32_t   _bus_err_count;			// CPT2
    volatile uint32_t   _slave_err_count;		// CPT3
    volatile uint32_t   _slave_msg_count;		// CPT4
    volatile uint32_t   _slave_no_resp_count;	// CPT5
    volatile uint32_t   _slave_nak_count;		// CPT6
    volatile uint32_t   _slave_busy_count;		// CPT7
    volatile uint32_t   _bus_overrun_count;		// CPT8
}rs485_diagnostic_t;

typedef struct{
	uint8_t					address;
	uint8_t					group_address;
	ring_buffer_t   		txPacket_rb;
	ring_buffer_t   		rxPacket_rb;
	rs485_channel_state_e	state;
	uint8_t					_not_respond_count;
	uint8_t 				_retry_count;
	bool					is_pre_sellect;
	bool					is_poll_active;
	bool					is_sellect_active;
	rs485timer_t			_sync_timer;
}rs485_channel_t;

struct rs485{
    uint8_t             address; 				// address of device
    rs485IF_t           *meIF;
    rs485_bus_mode_e 	bus_mode; 				// MASTER or SLAVE

    volatile uint16_t   _rx_byte_count;
    rs485_diagnostic_t  _diagnostic;

    bool				_is_bus_running;
    volatile bool 		_loopback_flag;
    volatile bool 		_eof_flag;				// end of frame flag
    volatile uint32_t	_tick_count;

	rs485_channel_t*	channel_list[RS485_MAX_CHANNEL_NUMBER];
	uint8_t 			num_of_channel;

	uint8_t             rxByte;
	uint8_t 			_cur_rxframe;
	uint8_t 			_cur_channel_id;
	uint8_t             _rx_size;
	uint8_t             rxframe[RS485_MAX_DATA_LENGTH];
	uint8_t             _tx_size;
	uint8_t             txframe[RS485_MAX_DATA_LENGTH];

	Packet_t 			rx_packet;
	Packet_t 			tx_packet;

	rs485_state_machine_data_t	sm_data;

};

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
void rs485_init(rs485_t *me, rs485IF_t *meIF, uint8_t deviceID, rs485_bus_mode_e bus_mode);
bool rs485_channel_init(rs485_t *me, uint8_t address, uint8_t *channel_id, bool is_tx_active, bool is_rx_active, uint8_t tx_cache_number, uint8_t rx_cache_number);

bool rs485_bus_start(rs485_t *me);

bool rs485_is_avaiable(rs485_t *me, uint8_t channel_id);
bool rs485_transmit(rs485_t *me, uint8_t channel_id, rs485_msg *msg);
bool rs485_receive(rs485_t *me, uint8_t channel_id, rs485_msg *msg);

void rs485_process(rs485_t *me);

uint8_t rs485_get_channelState(rs485_t *me, uint8_t channel_id);

/*************************** callback function **********************************/

void rs485_RxByte_callback(rs485_t *me);

