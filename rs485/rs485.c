/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include <stdlib.h>
#include "rs485.h"
#include "assert_handler.h"
#include "debug.h"
#include "standard.h"

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/

#define RS485_POLL_DELAY_US						250						// duration in us
#define RS485_REPLY_WAIT_US						10000
#define RS485_REPLY_DELAY_US					250
#define RS485_SEND_TIMEOUT_US					4000
#define RS485_CHANNEL_OFFLINE_SYNC_DURATION_US	5000000
#define RS485_CHANNEL_ONLINE_SYNC_DURATION_US	2000000

#define RS485_CHANNEL_NOT_RESPONSE_THRESHOLD	3


#define RS485_US_TO_TICKS(x)					(x / RS485_TICK_US) + ((x % RS485_TICK_US) > 0)
/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                              ENUMS
==================================================================================================*/
typedef enum {
	RS485_EVENT_LOOPBACK,
	RS485_EVENT_FRAME,
	RS485_EVENT_INIT,
	RS485_EVENT_JUMP_STATE,
    RS485_EVENT_TIMEOUT,
    RS485_EVENT_NONE
}rs485_state_event_e;

typedef enum{
	RS485_INIT_STATE,
    RS485_IDLE_STATE,
	RS485_POLL_STATE,
	RS485_SELLECT_STATE,
	RS485_STATE_END,
}rs485_state_e;

typedef enum{
	POLL_SEND_DELAY,
	POLL_SEND_WAIT,
	POLL_REPLY_WAIT,
	POLL_SEND_ACK_DELAY,
	POLL_SEND_ACK_WAIT
}rs485_poll_state_e;

typedef enum{
	SELLECT_RECEIVE_WAIT,
	SELLECT_SEND_WAIT,
	SELLECT_SEND_ACK_DELAY,
	SELLECT_REPLY_ACK_WAIT,
}rs485_sellect_state_e;

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

typedef void (*rs485StateEntry)(rs485_state_machine_data_t *data);
typedef void (*rs485StateFunc)(rs485_state_machine_data_t *data,uint8_t event);
typedef void (*rs485StateExit)(rs485_state_machine_data_t *data);

typedef struct {
	rs485StateEntry entry;
	rs485StateExit  exit;
	rs485StateFunc  state;
} rs485StateFunctions;

typedef enum{
    ADDR_ABSOLUTE,
    ADDR_COMMON,
    ADDR_INVALID
}address_valid_e;
/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
static void rs485_tx_current_frame(rs485_t *me);
static void rs485_tx_prepare(rs485_t *me, Frame_e frame_type, bool _is_instant_tx);
static uint8_t rs485_find_emptyChannel(rs485_t *me);
static bool rs485_go_next_channel(rs485_t *me);
bool rs485_channel_create(rs485_t *me, uint8_t address, uint8_t channel_id, bool is_tx_active, bool is_rx_active, uint8_t tx_cache_number, uint8_t rx_cache_number);
static void rs485_diagnostic_count(rs485_state_machine_data_t *data, uint8_t type);

/************************************ Rs485 State function ****************************************/
void Rs485StateInit(rs485_state_machine_data_t *data, uint8_t event);
void Rs485StateInitEntry(rs485_state_machine_data_t *data);
void Rs485StateInitExit(rs485_state_machine_data_t *data);

void Rs485MasterStateIdle(rs485_state_machine_data_t *data, uint8_t event);
void Rs485MasterStateIdleEntry(rs485_state_machine_data_t *data);
void Rs485MasterStateIdleExit(rs485_state_machine_data_t *data);

void Rs485MasterStatePoll(rs485_state_machine_data_t *data, uint8_t event);
void Rs485MasterStatePollEntry(rs485_state_machine_data_t *data);
void Rs485MasterStatePollExit(rs485_state_machine_data_t *data);

void Rs485MasterStateSellect(rs485_state_machine_data_t *data, uint8_t event);
void Rs485MasterStateSellectEntry(rs485_state_machine_data_t *data);
void Rs485MasterStateSellectExit(rs485_state_machine_data_t *data);

void Rs485SlaveStateIdle(rs485_state_machine_data_t *data, uint8_t event);
void Rs485SlaveStateIdleEntry(rs485_state_machine_data_t *data);
void Rs485SlaveStateIdleExit(rs485_state_machine_data_t *data);

void Rs485SlaveStatePoll(rs485_state_machine_data_t *data, uint8_t event);
void Rs485SlaveStatePollEntry(rs485_state_machine_data_t *data);
void Rs485SlaveStatePollExit(rs485_state_machine_data_t *data);

void Rs485SlaveStateSellect(rs485_state_machine_data_t *data, uint8_t event);
void Rs485SlaveStateSellectEntry(rs485_state_machine_data_t *data);
void Rs485SlaveStateSellectExit(rs485_state_machine_data_t *data);

/************************************ Rs485 State machine functions ****************************************/
static void rs485_timer_start(rs485_state_machine_data_t *data, rs485timer_t *timer, uint32_t tick);
static bool rs485_timer_timeout(rs485_state_machine_data_t *data, rs485timer_t *timer);
static void rs485_timer_clear(rs485_state_machine_data_t *data, rs485timer_t *timer);

static void rs485_state_machine_post_internal_event(void *data, uint8_t event);
static void rs485_state_transition(rs485_state_machine_data_t *data, uint8_t next_state);
static uint8_t rs485_process_input(rs485_state_machine_data_t *data);
static rs485StateFunctions* rs485_get_state_fp(rs485_state_machine_data_t *data, uint8_t state);
static uint8_t rs485_get_next_state(rs485_state_machine_data_t *data, uint8_t next_event);
static void rs485_state_machine_run(rs485_state_machine_data_t *data);
/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/
rs485StateFunctions rs485stateFunctions[] = {
	{Rs485StateInitEntry	  		, Rs485StateInitExit	    	, Rs485StateInit   	  		},
	{Rs485MasterStateIdleEntry	  	, Rs485MasterStateIdleExit	    , Rs485MasterStateIdle      },
	{Rs485MasterStatePollEntry	    , Rs485MasterStatePollExit	    , Rs485MasterStatePoll      },
	{Rs485MasterStateSellectEntry	, Rs485MasterStateSellectExit   , Rs485MasterStateSellect   },
	{Rs485SlaveStateIdleEntry		, Rs485SlaveStateIdleExit		, Rs485SlaveStateIdle 		},
	{Rs485SlaveStatePollEntry		, Rs485SlaveStatePollExit		, Rs485SlaveStatePoll 		},
	{Rs485SlaveStateSellectEntry	, Rs485SlaveStateSellectExit	, Rs485SlaveStateSellect   	},
};
/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/
static void rs485_rx_mode(rs485_t *me)
{
	rs485IF_t *tmpIF = me->meIF;
	tmpIF->rxMode();
}

static void rs485_tx_mode(rs485_t *me)
{
	rs485IF_t *tmpIF = me->meIF;
	tmpIF->txMode();
}

static uint8_t rs485_find_emptyChannel(rs485_t *me)
{
	uint8_t retVal = RS485_INVALID_CHANNEL_ID;
	uint8_t i;
	for(i = 0; i < RS485_MAX_CHANNEL_NUMBER; i++){
		if(me->channel_list[i] == NULL){
			retVal = i;
			break;
		}
	}
	return retVal;
}

static bool rs485_go_next_channel(rs485_t *me)
{
	bool retVal = false;
	uint8_t tmp_channel = me->_cur_channel_id;
	for(uint8_t i = 0 ; i < RS485_MAX_CHANNEL_NUMBER; i++)
	{
		tmp_channel++;
		if(tmp_channel >= RS485_MAX_CHANNEL_NUMBER)
		{
			tmp_channel = 0;
		}
		if(me->channel_list[tmp_channel] != NULL){
			me->_cur_channel_id = tmp_channel;
			retVal = true;
			break;
		}
	}
	return retVal;
}

static bool rs485_frame_avaiable(rs485_t *me)
{
    bool ret = false;
    if(true == rs485_timer_timeout(&me->sm_data,&me->sm_data._t35_timer)){
    	rs485_timer_clear(&me->sm_data,&me->sm_data._t35_timer);
    	me->meIF->uart_rx((uint8_t*)&me->rxByte,1U);
    	if(true == me->_loopback_flag)
    	{
    		me->_loopback_flag = false;
    		me->_rx_byte_count = 0;
    		rs485_state_machine_post_internal_event(&me->sm_data,RS485_EVENT_LOOPBACK);
    		rs485_rx_mode(me);
    	}else{
    		me->_cur_rxframe = packet_unframe(&me->rx_packet, me->rxframe , me->_rx_byte_count);
			me->_rx_byte_count = 0;
			ret = true;
    	}
    }
    return ret;
}

static void rs485_tx_current_frame(rs485_t *me)
{
	rs485IF_t *tmpIF = me->meIF;
	rs485_tx_mode(me);
	tmpIF->uart_tx(me->txframe,me->_tx_size);
	me->_loopback_flag = true;
	rs485_timer_start(&me->sm_data,(uint32_t*)&me->sm_data._t35_timer,RS485_US_TO_TICKS(RS485_T35_DURATION_US));
}

static void rs485_tx_prepare(rs485_t *me, Frame_e frame_type, bool _is_instant_tx)
{
	me->_tx_size = packet_frame(me->txframe,&me->tx_packet,frame_type);
	if(true == _is_instant_tx)
	{
		rs485_tx_current_frame(me);
	}
}

bool rs485_channel_create(rs485_t *me, uint8_t address, uint8_t channel_id, bool is_tx_active, bool is_rx_active, uint8_t tx_cache_number, uint8_t rx_cache_number)
{
	bool retVal = false;
	me->channel_list[channel_id] = (rs485_channel_t*)malloc(sizeof(rs485_channel_t));
	if(me->channel_list[channel_id] != NULL){
		if(me->bus_mode == MASTER_MODE){
			if(true == is_tx_active)
			{
				ring_buffer_init(&me->channel_list[channel_id]->txPacket_rb,tx_cache_number,sizeof(Packet_t));
			}
			if(true == is_rx_active){
				ring_buffer_init(&me->channel_list[channel_id]->rxPacket_rb,rx_cache_number,sizeof(Packet_t));
			}
			me->channel_list[channel_id]->is_poll_active 	= is_rx_active;
			me->channel_list[channel_id]->is_sellect_active = is_tx_active;
		}else if(me->bus_mode == SLAVE_MODE){
			if(true == is_tx_active){
				ring_buffer_init(&me->channel_list[channel_id]->txPacket_rb,tx_cache_number,sizeof(Packet_t));
			}
			if(true == is_rx_active){
				ring_buffer_init(&me->channel_list[channel_id]->rxPacket_rb,rx_cache_number,sizeof(Packet_t));
			}
			me->channel_list[channel_id]->is_poll_active 	= is_tx_active;
			me->channel_list[channel_id]->is_sellect_active = is_rx_active;
		}
		me->channel_list[channel_id]->address = address;
		me->channel_list[channel_id]->state = RS485_CHANNEL_INIT_STATE;
		me->channel_list[channel_id]->_not_respond_count = 0;
		me->num_of_channel++;
		retVal = true;
	}
	return retVal;
}

static void rs485_channel_state_transition(rs485_state_machine_data_t *data, uint8_t state)
{
	rs485_channel_t *cur_channel;
	cur_channel = data->common->channel_list[data->common->_cur_channel_id];
	if(cur_channel->state != state)
	{
		switch(state){
			case RS485_CHANNEL_INIT_STATE:
				break;
			case RS485_CHANNEL_OFFLINE_STATE:
				rs485_timer_start(data,&cur_channel->_sync_timer,RS485_US_TO_TICKS(RS485_CHANNEL_OFFLINE_SYNC_DURATION_US));
				break;
			case RS485_CHANNEL_ONLINE_STATE:
				if(true == cur_channel->is_poll_active){
					rs485_timer_clear(data,&cur_channel->_sync_timer);
				}else{
					rs485_timer_start(data,&cur_channel->_sync_timer,RS485_US_TO_TICKS(RS485_CHANNEL_ONLINE_SYNC_DURATION_US));
				}
				break;
			case RS485_CHANNEL_NONE_STATE:
			default:
				break;
		}
		cur_channel->state = state;
	}
}

static void rs485_diagnostic_count(rs485_state_machine_data_t *data, uint8_t type)
{
	switch(type)
	{
	case BUS_MSG_COUNT:
		data->common->_diagnostic._bus_msg_count++;
		break;
	case BUS_ERR_COUNT:
		DEV_Digital_Toggle(TRIGGER3_PORT_PIN);
		data->common->_diagnostic._slave_err_count++;
		break;
	case BUS_OVERRUN_COUNT:
		data->common->_diagnostic._bus_overrun_count++;
		break;
	default:
		break;
	}
}
/************************************ Rs485 State machine functions ****************************************/
#define RS485_TIMER_CLEARED (0u)

static void rs485_timer_start(rs485_state_machine_data_t *data, rs485timer_t *timer, uint32_t tick)
{
    *timer = data->common->_tick_count + tick;
}

static bool rs485_timer_timeout(rs485_state_machine_data_t *data, rs485timer_t *timer)
{
    if (*timer == RS485_TIMER_CLEARED) {
        return false;
    }
    int32_t left = *timer - data->common->_tick_count;
    return (left < 0);
}

static void rs485_timer_clear(rs485_state_machine_data_t *data, rs485timer_t *timer)
{
    *timer = RS485_TIMER_CLEARED;
}


static void rs485_state_machine_post_internal_event(void *data, uint8_t event)
{
	rs485_state_machine_data_t *sm = (rs485_state_machine_data_t*)data;
	sm->internal_event = event;
}

static void rs485_state_transition(rs485_state_machine_data_t *data, uint8_t next_state)
{
	data->next_state = next_state;
	rs485_state_machine_post_internal_event(data, RS485_EVENT_JUMP_STATE);
}

static rs485StateFunctions* rs485_get_state_fp(rs485_state_machine_data_t *data, uint8_t state)
{
	rs485StateFunctions *retVal = NULL;
	switch(state)
	{
		case RS485_INIT_STATE:
			retVal = &rs485stateFunctions[RS485_INIT_STATE];
			break;
		case RS485_IDLE_STATE:
		case RS485_POLL_STATE:
		case RS485_SELLECT_STATE:
			switch(data->common->bus_mode)
			{
				case MASTER_MODE:
					retVal = &rs485stateFunctions[state];
					break;
				case SLAVE_MODE:
					retVal = &rs485stateFunctions[RS485_SELLECT_STATE + state];
					break;
			}
			break;
		default:
			return NULL;
	}
	return retVal;
}

static uint8_t rs485_get_next_state(rs485_state_machine_data_t *data, uint8_t next_event)
{
	uint8_t next_state;
	next_state = data->cur_state;
	if(next_event == RS485_EVENT_INIT){
		next_state = RS485_INIT_STATE;
	}else if(next_event == RS485_EVENT_JUMP_STATE){
		next_state = data->next_state;
	}
	return next_state;
}

static uint8_t rs485_process_input(rs485_state_machine_data_t *data)
{
	/*
	 * what we do here ?
	 * put this process on timer interrupt trigger every 250usec
	 * 1. increase Frame Silent Interval Counter to keep track of a 3.5 character delay
	 * 2. increase response timeout counter
	 *
	 * */
	uint8_t ev_ret = RS485_EVENT_NONE;
	if(data->common->_is_bus_running == false){
		return ev_ret;
	}
	data->common->_tick_count++;
	if(true == rs485_frame_avaiable(data->common)){
		ev_ret = RS485_EVENT_FRAME;
	}else if(data->internal_event < RS485_EVENT_NONE){
		ev_ret = data->internal_event;
		data->internal_event = RS485_EVENT_NONE;
	}else if(rs485_timer_timeout(data, &data->timer)){
		rs485_timer_clear(data,&data->timer);
		ev_ret = RS485_EVENT_TIMEOUT;
	}
	return ev_ret;
}

static void rs485_state_machine_run(rs485_state_machine_data_t *data)
{
	uint8_t event = RS485_EVENT_NONE;
	uint8_t next_state;
	event = rs485_process_input(data);
	if(RS485_EVENT_NONE != event)
	{
		next_state = rs485_get_next_state(data,event);
		if(RS485_EVENT_INIT == event)
		{
			data->cur_state = next_state;
			rs485_get_state_fp(data, data->cur_state)->entry(data);
		}else
		{
			if(next_state != data->cur_state)
			{
				//state transiton process : exit current state and entry next state
				rs485_get_state_fp(data, data->cur_state)->exit(data);
				data->cur_state = next_state;
				rs485_get_state_fp(data, data->cur_state)->entry(data);
			}
		}
	}
	rs485_get_state_fp(data, data->cur_state)->state(data,event);
}
/************************************ Rs485 State function ****************************************/
///////////////////////////// MASTER PROCESS ///////////////////////////////////////
// INIT
void Rs485StateInit(rs485_state_machine_data_t *data, uint8_t event)
{
	switch(event)
	{
		case RS485_EVENT_FRAME:
			rs485_state_transition(data, RS485_IDLE_STATE);
			break;
		default:
			break;
	}
}
void Rs485StateInitEntry(rs485_state_machine_data_t *data)
{
	;
}
void Rs485StateInitExit(rs485_state_machine_data_t *data)
{
	;
}

///////////////////////////// MASTER PROCESS ///////////////////////////////////////
// IDLE
void Rs485MasterStateIdle(rs485_state_machine_data_t *data, uint8_t event)
{
	rs485_channel_t *cur_channel;
	if(true == rs485_go_next_channel(data->common))
	{
		cur_channel = data->common->channel_list[data->common->_cur_channel_id];
		switch(cur_channel->state)
		{
			case RS485_CHANNEL_INIT_STATE:
				rs485_state_transition(data, RS485_POLL_STATE);
				break;
			case RS485_CHANNEL_OFFLINE_STATE:
				if(true == rs485_timer_timeout(data, &cur_channel->_sync_timer)){
					rs485_timer_start(data,&cur_channel->_sync_timer,RS485_US_TO_TICKS(RS485_CHANNEL_OFFLINE_SYNC_DURATION_US));
					rs485_state_transition(data, RS485_POLL_STATE);
				}
				break;
			case RS485_CHANNEL_ONLINE_STATE:
				if((true == cur_channel->is_sellect_active) && (BUFFER_NOT_EMPTY == ring_buffer_is_empty(&cur_channel->txPacket_rb)) && ((false == cur_channel->is_pre_sellect) || (false == cur_channel->is_poll_active))){
					rs485_state_transition(data, RS485_SELLECT_STATE);
					cur_channel->is_pre_sellect = true;
				}else{
					if(true == cur_channel->is_pre_sellect)
					{
						cur_channel->is_pre_sellect = false;
					}
					if(true == cur_channel->is_poll_active){
						rs485_state_transition(data, RS485_POLL_STATE);
					}else{
						if(true == rs485_timer_timeout(data, &cur_channel->_sync_timer)){
							rs485_timer_start(data,&cur_channel->_sync_timer,RS485_US_TO_TICKS(RS485_CHANNEL_ONLINE_SYNC_DURATION_US));
							rs485_state_transition(data, RS485_POLL_STATE);
						}
					}
				}
				break;
			case RS485_CHANNEL_NONE_STATE:
			default:
				break;
		}
	}
}
void Rs485MasterStateIdleEntry(rs485_state_machine_data_t *data)
{
	;
}
void Rs485MasterStateIdleExit(rs485_state_machine_data_t *data)
{
	rs485_timer_clear(data,&data->timer);
}

// POL
void Rs485MasterStatePoll(rs485_state_machine_data_t *data, uint8_t event)
{
	rs485_channel_t *cur_channel;
	cur_channel = data->common->channel_list[data->common->_cur_channel_id];
	switch(data->cur_poll_state)
	{
		case POLL_SEND_DELAY:
			switch(event)
			{
				case RS485_EVENT_TIMEOUT:
					rs485_tx_current_frame(data->common);
					data->cur_poll_state = POLL_SEND_WAIT;
					break;
				default:
					break;
			}
			break;
		case POLL_SEND_WAIT:
			switch(event)
			{
				case RS485_EVENT_LOOPBACK:
					data->cur_poll_state = POLL_REPLY_WAIT;
					rs485_timer_start(data,&data->timer,RS485_US_TO_TICKS(RS485_REPLY_WAIT_US));
					break;
				default:
					break;
			}
			break;
		case POLL_REPLY_WAIT:
			switch(event)
			{
				case RS485_EVENT_FRAME:
					if(cur_channel->state != RS485_CHANNEL_ONLINE_STATE)
					{
						rs485_channel_state_transition(data,RS485_CHANNEL_ONLINE_STATE);
					}
					cur_channel->_not_respond_count = 0;
					switch(data->common->_cur_rxframe)
					{
						case EOT_FRAME:
							rs485_state_transition(data, RS485_IDLE_STATE);
							break;
						case RESPOND_FRAME:
							rs485_diagnostic_count(data,BUS_MSG_COUNT);
							if(cur_channel->address == data->common->rx_packet.address){
								ring_buffer_put(&cur_channel->rxPacket_rb, &data->common->rx_packet);
								data->cur_poll_state = POLL_SEND_ACK_DELAY;
								rs485_tx_prepare(data->common,ACK_FRAME,false);
								rs485_timer_start(data,&data->timer,RS485_US_TO_TICKS(RS485_REPLY_DELAY_US));
							}else{
								rs485_state_transition(data, RS485_IDLE_STATE);
							}
							break;
						case ERROR_FRAME:
							rs485_diagnostic_count(data,BUS_ERR_COUNT);
							rs485_tx_prepare(data->common,NACK_FRAME,false);
							data->cur_poll_state = POLL_SEND_ACK_DELAY;
							rs485_timer_start(data,&data->timer,RS485_US_TO_TICKS(RS485_REPLY_DELAY_US));
							break;
						default:
//							rs485_state_transition(data, RS485_IDLE_STATE);
							break;
					}
					break;
				case RS485_EVENT_TIMEOUT:
					cur_channel->_not_respond_count++;
					if(cur_channel->_not_respond_count > RS485_CHANNEL_NOT_RESPONSE_THRESHOLD){
						rs485_channel_state_transition(data,RS485_CHANNEL_OFFLINE_STATE);
					}
					rs485_state_transition(data, RS485_IDLE_STATE);
					break;
				default:
					break;
			}
			break;
		case POLL_SEND_ACK_DELAY:
			switch(event)
			{
				case RS485_EVENT_TIMEOUT:
					rs485_tx_current_frame(data->common);
					data->cur_poll_state = POLL_REPLY_WAIT;
					rs485_timer_start(data,&data->timer,RS485_US_TO_TICKS(RS485_REPLY_WAIT_US));
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}
void Rs485MasterStatePollEntry(rs485_state_machine_data_t *data)
{
	rs485_channel_t *cur_channel;

	cur_channel = data->common->channel_list[data->common->_cur_channel_id];
	data->common->tx_packet.address = cur_channel->address;
	rs485_tx_prepare(data->common,POLL_FRAME,false);
	data->cur_poll_state = POLL_SEND_DELAY;
	rs485_timer_start(data,&data->timer,RS485_US_TO_TICKS(RS485_POLL_DELAY_US));

}
void Rs485MasterStatePollExit(rs485_state_machine_data_t *data)
{
	rs485_timer_clear(data,&data->timer);
	data->cur_poll_state = POLL_SEND_DELAY;
}

// SELLECT
void Rs485MasterStateSellect(rs485_state_machine_data_t *data, uint8_t event)
{
	rs485_channel_t *cur_channel;
	cur_channel = data->common->channel_list[data->common->_cur_channel_id];
	switch(data->cur_sellect_state)
	{
		case SELLECT_SEND_WAIT:
			switch(event)
			{
				case RS485_EVENT_LOOPBACK:
					data->cur_sellect_state = SELLECT_REPLY_ACK_WAIT;
					rs485_timer_start(data,&data->timer,RS485_US_TO_TICKS(RS485_REPLY_WAIT_US));
					break;
				default:
					break;
			}
			break;
		case SELLECT_REPLY_ACK_WAIT:
			switch(event)
			{
				case RS485_EVENT_FRAME:
					switch(data->common->_cur_rxframe)
					{
						case ACK_FRAME:
							rs485_state_transition(data, RS485_IDLE_STATE);
							break;
						case NACK_FRAME:
							cur_channel->_retry_count++;
							if(cur_channel->_retry_count > MAX_RETRY_NUMBER){
								rs485_state_transition(data, RS485_IDLE_STATE);
							}else{
								rs485_tx_current_frame(data->common);
								data->cur_sellect_state = SELLECT_SEND_WAIT;
							}
							rs485_diagnostic_count(data,BUS_ERR_COUNT);
							break;
						default:
//							rs485_state_transition(data, RS485_IDLE_STATE);
							break;
					}
					break;
				case RS485_EVENT_TIMEOUT:
					rs485_diagnostic_count(data,BUS_OVERRUN_COUNT);
					rs485_state_transition(data, RS485_IDLE_STATE);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}
void Rs485MasterStateSellectEntry(rs485_state_machine_data_t *data)
{
	rs485_channel_t *cur_channel;
	data->cur_sellect_state = SELLECT_SEND_WAIT;
	cur_channel = data->common->channel_list[data->common->_cur_channel_id];
	ring_buffer_get(&cur_channel->txPacket_rb, &data->common->tx_packet);
	data->common->tx_packet.address = cur_channel->address;
	cur_channel->_retry_count = 0;
	rs485_tx_prepare(data->common,SELLECT_FRAME,true);
	rs485_diagnostic_count(data,BUS_MSG_COUNT);
}
void Rs485MasterStateSellectExit(rs485_state_machine_data_t *data)
{
	data->cur_sellect_state = SELLECT_SEND_WAIT;
	rs485_timer_clear(data,&data->timer);
}

///////////////////////////// SLAVE PROCESS ///////////////////////////////////////

void Rs485SlaveStateIdle(rs485_state_machine_data_t *data, uint8_t event)
{
	rs485_channel_t *cur_channel;
	cur_channel = data->common->channel_list[0];
	rs485_state_transition(data, RS485_SELLECT_STATE);
	rs485_timer_start(data,&cur_channel->_sync_timer,RS485_US_TO_TICKS(RS485_CHANNEL_ONLINE_SYNC_DURATION_US));
}
void Rs485SlaveStateIdleEntry(rs485_state_machine_data_t *data)
{
	;
}
void Rs485SlaveStateIdleExit(rs485_state_machine_data_t *data)
{
	;
}

void Rs485SlaveStatePoll(rs485_state_machine_data_t *data, uint8_t event)
{
	rs485_channel_t *cur_channel;
	cur_channel = data->common->channel_list[0];
	switch(data->cur_poll_state)
	{
		case POLL_SEND_DELAY:
			switch(event)
			{
				case RS485_EVENT_TIMEOUT:
					rs485_tx_current_frame(data->common);
					data->cur_poll_state = POLL_SEND_WAIT;
					break;
				default:
					break;
			}
			break;
		case POLL_SEND_WAIT:
			switch(event)
			{
				case RS485_EVENT_LOOPBACK:
					data->cur_poll_state = POLL_REPLY_WAIT;
					rs485_timer_start(data,&data->timer,RS485_US_TO_TICKS(RS485_REPLY_WAIT_US));
					break;
				default:
					break;
			}
			break;
		case POLL_REPLY_WAIT:
			switch(event)
			{
				case RS485_EVENT_FRAME:
					switch(data->common->_cur_rxframe)
					{
						case ACK_FRAME:
							rs485_tx_prepare(data->common,EOT_FRAME,false);
							data->cur_poll_state = POLL_SEND_ACK_DELAY;
							rs485_timer_start(data,&data->timer,RS485_US_TO_TICKS(RS485_REPLY_DELAY_US));
							break;
						case NACK_FRAME:
							cur_channel->_retry_count++;
							if(cur_channel->_retry_count > MAX_RETRY_NUMBER){
								rs485_tx_prepare(data->common,EOT_FRAME,false);
								data->cur_poll_state = POLL_SEND_ACK_DELAY;
								rs485_timer_start(data,&data->timer,RS485_US_TO_TICKS(RS485_REPLY_DELAY_US));
							}else{
								rs485_tx_current_frame(data->common);
								data->cur_poll_state = POLL_SEND_WAIT;
							}
							break;
						case POLL_FRAME:
						case SELLECT_FRAME:
						case RESPOND_FRAME:
							rs485_state_transition(data, RS485_SELLECT_STATE);
							break;
						case ERROR_FRAME:
						default:
							//rs485_state_transition(data, RS485_SELLECT_STATE);
							break;
					}
					break;
				case RS485_EVENT_TIMEOUT:
					cur_channel->_not_respond_count++;
					rs485_state_transition(data, RS485_SELLECT_STATE);
					break;
				default:
					break;
			}
			break;
		case POLL_SEND_ACK_DELAY:
			switch(event)
			{
				case RS485_EVENT_TIMEOUT:
					rs485_tx_current_frame(data->common);
					data->cur_poll_state = POLL_SEND_ACK_WAIT;
					break;
				case RS485_EVENT_FRAME:
					rs485_state_transition(data, RS485_SELLECT_STATE);
					break;
				default:
					break;
			}
			break;
		case POLL_SEND_ACK_WAIT:
			switch(event)
			{
				case RS485_EVENT_FRAME:
				case RS485_EVENT_LOOPBACK:
				case RS485_EVENT_TIMEOUT:
					rs485_state_transition(data, RS485_SELLECT_STATE);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}
void Rs485SlaveStatePollEntry(rs485_state_machine_data_t *data)
{
	rs485_channel_t *cur_channel;
	cur_channel = data->common->channel_list[0];
	if((true == cur_channel->is_poll_active) && (false == ring_buffer_is_empty(&cur_channel->txPacket_rb)))
	{
		ring_buffer_get(&cur_channel->txPacket_rb, &data->common->tx_packet);
		data->common->tx_packet.address = data->common->address;
		cur_channel->_retry_count = 0;
		rs485_tx_prepare(data->common,RESPOND_FRAME,false);
		data->cur_poll_state = POLL_SEND_DELAY;
		rs485_timer_start(data,&data->timer,RS485_US_TO_TICKS(RS485_REPLY_DELAY_US));
	}else{
		rs485_tx_prepare(data->common,EOT_FRAME,false);
		data->cur_poll_state = POLL_SEND_ACK_DELAY;
		rs485_timer_start(data,&data->timer,RS485_US_TO_TICKS(RS485_REPLY_DELAY_US));
	}
}
void Rs485SlaveStatePollExit(rs485_state_machine_data_t *data)
{
	data->cur_poll_state = POLL_SEND_DELAY;
	rs485_timer_clear(data,&data->timer);
}

void Rs485SlaveStateSellect(rs485_state_machine_data_t *data, uint8_t event)
{
	rs485_channel_t *cur_channel;
	address_valid_t address_valid;
	cur_channel = data->common->channel_list[0];
	if(true == rs485_timer_timeout(data, &cur_channel->_sync_timer)){
		cur_channel->state = RS485_CHANNEL_OFFLINE_STATE;
	}
	switch(data->cur_sellect_state)
	{
		case SELLECT_RECEIVE_WAIT:
			switch(event)
			{
				case RS485_EVENT_FRAME:
					switch(data->common->_cur_rxframe)
					{
						case POLL_FRAME:
							/* Polling masssage*/
							/* EOT(1B) | SA(1B) | POL(1B) */
							/* SA(1B) : slave address check valid*/
							address_valid = rs485_address_validate(data->common->address, data->common->rx_packet.address);
							if((address_valid.device_type_valid == true) && (address_valid.physical_addr_valid == true) && (address_valid.is_reply == true))
							{
								if(cur_channel->state != RS485_CHANNEL_ONLINE_STATE){
									cur_channel->state = RS485_CHANNEL_ONLINE_STATE;
								}
								rs485_state_transition(data, RS485_POLL_STATE);
								rs485_timer_start(data,&cur_channel->_sync_timer,RS485_US_TO_TICKS(RS485_CHANNEL_ONLINE_SYNC_DURATION_US));
							}else{

							}
							break;
						case SELLECT_FRAME:
							/* Sellecting masssage*/
							/* EOT(1B) | STX(1B) | SA(1B) | OP(1B) | Data(nB) | ETX(1B) | BCC(1B) */
							/* SA(1B) : slave address check valid*/
							address_valid = rs485_address_validate(data->common->address, data->common->rx_packet.address);
							if((address_valid.device_type_valid == true) && (address_valid.physical_addr_valid == true))
							{
								ring_buffer_put(&cur_channel->rxPacket_rb, &data->common->rx_packet);
								if(address_valid.is_reply == true)
								{
									rs485_tx_prepare(data->common,ACK_FRAME,false);
									data->cur_sellect_state = SELLECT_SEND_ACK_DELAY;
									rs485_timer_start(data,&data->timer,RS485_US_TO_TICKS(RS485_REPLY_DELAY_US));
								}
							}
							break;
						case ERROR_FRAME:
							/* Sellecting masssage*/
							/* EOT(1B) | STX(1B) | SA(1B) | OP(1B) | Data(nB) | ETX(1B) | BCC(1B) */
							/* SA(1B) : slave address check valid*/
							address_valid = rs485_address_validate(data->common->address, data->common->rx_packet.address);
							if((address_valid.device_type_valid == true) && (address_valid.physical_addr_valid == true))
							{
								if(address_valid.is_reply == true)
								{
									rs485_tx_prepare(data->common,NACK_FRAME,false);
									data->cur_sellect_state = SELLECT_SEND_ACK_DELAY;
									rs485_timer_start(data,&data->timer,RS485_US_TO_TICKS(RS485_REPLY_DELAY_US));
								}
							}
							break;
						default:
							break;
					}
					break;
				default:
					break;
			}
			break;
		case SELLECT_SEND_ACK_DELAY:
			switch(event)
			{
				case RS485_EVENT_TIMEOUT:
					rs485_tx_current_frame(data->common);
					data->cur_sellect_state = SELLECT_RECEIVE_WAIT;
					break;
				case RS485_EVENT_FRAME:
					data->cur_sellect_state = SELLECT_RECEIVE_WAIT;
					rs485_timer_clear(data,&data->timer);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}
void Rs485SlaveStateSellectEntry(rs485_state_machine_data_t *data)
{
	data->cur_sellect_state = SELLECT_RECEIVE_WAIT;
}
void Rs485SlaveStateSellectExit(rs485_state_machine_data_t *data)
{
	data->cur_sellect_state = SELLECT_RECEIVE_WAIT;
	rs485_timer_clear(data,&data->timer);
}

/*==================================================================================================
*                                        GLOBAL FUNCTIONS
==================================================================================================*/
void rs485_init(rs485_t *me, rs485IF_t *meIF, uint8_t deviceID, rs485_bus_mode_e bus_mode)
{
    ASSERT((me!=NULL)&&(meIF!=NULL));

    uint8_t i = 0;
    me->meIF          	= meIF;
    me->address         = deviceID;
    me->bus_mode		= bus_mode;

    for(i = 0; i < RS485_MAX_CHANNEL_NUMBER; i++)
    {
    	me->channel_list[i] = NULL;
    }
    me->_tick_count 	= 0;
    me->_is_bus_running	= false;
    me->_cur_channel_id = RS485_MAX_CHANNEL_NUMBER;
    me->num_of_channel 	= 0;
    me->_loopback_flag 	= false;

    /*** state mạchine init ****/
    me->sm_data.common				= me;
    me->sm_data.cur_state 			= RS485_INIT_STATE;
    me->sm_data.cur_poll_state 		= POLL_SEND_WAIT;
    me->sm_data.cur_sellect_state 	= SELLECT_SEND_WAIT;
    me->sm_data.internal_event 		= RS485_EVENT_INIT;
    me->sm_data.timer 				= 0;

}

bool rs485_channel_init(rs485_t *me, uint8_t address, uint8_t *channel_id, bool is_tx_active, bool is_rx_active, uint8_t tx_cache_number, uint8_t rx_cache_number){
	uint8_t retVal = false;
	*channel_id = rs485_find_emptyChannel(me);
	if(*channel_id < RS485_INVALID_CHANNEL_ID){
		retVal = rs485_channel_create(me,address,*channel_id,is_tx_active,is_rx_active,tx_cache_number,rx_cache_number);
	}
	return retVal;
}

bool rs485_bus_start(rs485_t *me)
{
	uint8_t retVal = false;
	rs485_timer_start(&me->sm_data,(uint32_t*)&me->sm_data._t35_timer,RS485_US_TO_TICKS(RS485_T35_DURATION_US));
	me->_is_bus_running = true;

	/*** start receive data *****/
	me->meIF->rxMode();
	me->_rx_byte_count = 0;
	me->meIF->uart_rx((uint8_t*)&me->rxByte,1U);                       /* receive 1 byte */
	return retVal;
}

bool rs485_transmit(rs485_t *me, uint8_t channel_id,  rs485_msg *msg)
{
    ASSERT(me!=NULL);
    bool retVal = false;

    if((me->channel_list[channel_id] != NULL) && (channel_id < RS485_MAX_CHANNEL_NUMBER) && (RS485_CHANNEL_ONLINE_STATE == rs485_get_channelState(me,channel_id)))
    {
    	if(!ring_buffer_is_full(&me->channel_list[channel_id]->txPacket_rb))
		{
    		retVal = true;
			ring_buffer_put(&me->channel_list[channel_id]->txPacket_rb,msg);
		}
    }
    return retVal;
}

bool rs485_receive(rs485_t *me, uint8_t channel_id, rs485_msg *msg)
{
    ASSERT(me!=NULL);
    bool retVal = false;

    if((me->channel_list[channel_id] != NULL) && (channel_id < RS485_MAX_CHANNEL_NUMBER))
	{
    	if(!ring_buffer_is_empty(&me->channel_list[channel_id]->rxPacket_rb))
		{
    		retVal = true;
    		ring_buffer_get(&me->channel_list[channel_id]->rxPacket_rb,msg);
		}
	}
    return retVal;
}

bool rs485_is_avaiable(rs485_t *me, uint8_t channel_id)
{
    bool retVal = false;
    if((me->channel_list[channel_id] != NULL) && (channel_id < RS485_MAX_CHANNEL_NUMBER))
	{
    	if(!ring_buffer_is_empty(&me->channel_list[channel_id]->rxPacket_rb))
		{
    		retVal = true;
		}
	}
    return retVal;
}

uint8_t rs485_get_channelState(rs485_t *me, uint8_t channel_id)
{
	return me->channel_list[channel_id]->state;
}

/* this process each 250us */
void rs485_process(rs485_t *me)
{
	if(true == me->_is_bus_running)
	{
		rs485_state_machine_run(&me->sm_data);
	}
}

/****************************** callback ***********************************/
void rs485_RxByte_callback(rs485_t *me)
{
	/*
	 * what we do here?
	 * 1. fill buffer with rx Byte; increment number of rxbyte
	 * 2. reset response timeout counter & frame silent interval counter (t3.5)
	 *
	 * */
    rs485IF_t *tmpIF = me->meIF;
	me->rxframe[me->_rx_byte_count] = me->rxByte;
	me->_rx_byte_count++;
	rs485_timer_start(&me->sm_data,(uint32_t*)&me->sm_data._t35_timer,RS485_US_TO_TICKS(RS485_T35_DURATION_US));

	tmpIF->uart_rx((uint8_t*)&me->rxByte,1U); /* receive 1 byte */
}
