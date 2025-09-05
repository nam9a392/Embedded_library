#ifndef RS_PACKET_H
#define RS_PACKET_H

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include <stdint.h>
#include <stdbool.h>
/*==================================================================================================
*                                              ENUMS
==================================================================================================*/
typedef enum
{
    ACK_FRAME,
    NACK_FRAME,
    EOT_FRAME,
    POLL_FRAME,
    SELLECT_FRAME,
    RESPOND_FRAME,
    ERROR_FRAME, // No error
    NONE_FRAME
}Frame_e;

typedef enum
{
    SYNC,               /**< Looks for a sync character */
    OP_CODE,            /**< Receives the op code */
    DATA_LENGTH,        /**< Receives the data size */
    DATA,               /**< Receives the packet data */
    CHECKSUM,           /**< Receives the checksum */
    PACKET_STATE_END    /**< Defines max state allowed */
} PacketRxState_e;

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/

#define RS485_BROADCAST_ADDRESS				0xff
#define MAX_PACKET_LENGTH       			50
/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct{
    uint8_t address;
    uint8_t opcode;
    uint8_t data[MAX_PACKET_LENGTH];
    uint16_t length;
}Packet_t;

typedef struct{
    PacketRxState_e state;
}RsPacket;

typedef struct{
    bool device_type_valid;        	// true  |  false
    bool physical_addr_valid;       // true  |  false
    bool is_reply;       	        // true  |  false   : sub_address | main_address
}address_valid_t;
/*==================================================================================================
*                                  GLOBAL FUNCTION PROTOTYPES
==================================================================================================*/
//void packet_parser_init(RsPacket *rspacket);

uint16_t packet_frame(uint8_t *frame, Packet_t *packet, Frame_e frame_type);

Frame_e packet_unframe(Packet_t *packet, uint8_t *frame, uint16_t len);

address_valid_t rs485_address_validate(uint8_t address_src, uint8_t address_dest);
#endif /* RS_PACKET_H */
