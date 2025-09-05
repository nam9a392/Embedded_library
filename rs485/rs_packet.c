/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "rs_packet.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "assert_handler.h"
/*==================================================================================================
*                                              ENUMS
==================================================================================================*/

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define STX        0x02U
#define ETX        0x03U
#define EOT        0x04U
#define POL        0x05U
#define ACK        0x06U
#define NACK       0x15U
/*==================================================================================================
*                                              CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                    LOCAL FUNCTIONS PROTOTYPES
==================================================================================================*/
static uint8_t CheckSum(uint8_t* pData, uint16_t Length);
/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/
static uint8_t CheckSum(uint8_t* pData, uint16_t Length)
{
    uint16_t CheckSumResult = 0;
    uint16_t i=0;
    for(i=0;i<Length;i++)
    {
        CheckSumResult ^= pData[i];
    }
    return CheckSumResult;
}

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

Frame_e packet_unframe(Packet_t *packet, uint8_t *frame, uint16_t len)
{
    ASSERT((packet!=NULL)&&(frame!=NULL));
    Frame_e ret = ERROR_FRAME;
    uint8_t CheckSumResult = 0;
    packet->address = 0xFF;
    if(len > 0)
    {
		switch(frame[0])
		{
			case ACK:
			{
				/* ACK massage*/
				/* | ACK (1B)| */
				if(len == 1)
				{
					/* massage sendding valid and do notthing */
					ret = ACK_FRAME;
				}
				break;
			}
			case NACK:
			{
				/* NACK massage*/
				/* | NACK (1B)| */
				if(len == 1)
				{
					ret = NACK_FRAME;
				}
				break;
			}
			case EOT:
			{
				/* Polling masssage*/
				/* EOT(1B) | SA(1B) | POL(1B) */
				/* SA(1B) : slave address check valid*/
				if(len == 1)
				{
					ret = EOT_FRAME;
				}else if(len == 3)
				{
					/* Polling masssage*/
					/* EOT(1B) | SA(1B) | POL(1B) */
					if(frame[2] == POL)
					{
						packet->address = frame[1];
						ret = POLL_FRAME;
					}
				}else if(len > 3)
				{
					/* Sellecting masssage*/
					/* EOT(1B) | STX(1B) | SA(1B) | OP(1B) | Data(nB) | ETX(1B) | BCC(1B) */
					if((frame[1] == STX) && (frame[len - 2] == ETX))
					{
						CheckSumResult = CheckSum((uint8_t*)&frame[2],len - 3);
						packet->address = frame[2];
						packet->opcode  = frame[3];
						packet->length  = len - 6;
						if(CheckSumResult == frame[len - 1])
						{
							memcpy(packet->data,&frame[4],packet->length);
							ret = SELLECT_FRAME;
						}else
						{
							ret = ERROR_FRAME;
						}
					}else
					{
						ret = NONE_FRAME;
					}
				}
				break;
			}
			case STX:
			{
				if(len > 3)
				{
					/* Respond masssage*/
					/* STX(1B) | SA(1B) | OP(1B) | Data(nB) | ETX(1B) | BCC(1B) */
					if(frame[len - 2] == ETX)
					{
						CheckSumResult = CheckSum((uint8_t*)&frame[1],len - 2);
						packet->address = frame[1];
						packet->opcode  = frame[2];
						packet->length  = len - 5;
						if(CheckSumResult == frame[len - 1])
						{
							memcpy(packet->data,&frame[3],packet->length);
							ret = RESPOND_FRAME;
						}else{
							ret = NONE_FRAME;
						}
					}
				}else{
					ret = NONE_FRAME;
				}
				break;
			}
			default:
				/* data not valid respond ERROR_FRAME */
				ret = NONE_FRAME;
				break;
		}
    }
    return ret;
}

uint16_t packet_frame(uint8_t *frame, Packet_t *packet, Frame_e frame_type)
{
    ASSERT(frame!=NULL);
    uint16_t i = 0;
    uint16_t j = 0;
    uint16_t FrameLength = 0;
    uint16_t sumLength = 0;
    if(frame_type <= NONE_FRAME)
    {
        switch(frame_type)
        {
            case ACK_FRAME:
            {
                frame[0] = ACK;
                FrameLength = 1;
                break;
            }
            case NACK_FRAME:
            {
                frame[0] = NACK;
                FrameLength = 1;
                break;
            }
            case EOT_FRAME:
            {
                frame[0] = EOT;
                FrameLength = 1;
                break;
            }
            case RESPOND_FRAME:
            {
                ASSERT(packet!=NULL);
                frame[j++] = STX;
                frame[j++] = packet->address;
                frame[j++] = packet->opcode;
                for(i=0;i<packet->length;i++)
                {
                    frame[j++] = packet->data[i];
                }
                frame[j++] = ETX;
                sumLength = j - 1;
                frame[j++] = CheckSum(&frame[1],sumLength);
                FrameLength = j;
                break;
            }
            case POLL_FRAME:
            	frame[j++] = EOT;
            	frame[j++] = packet->address;
            	frame[j++] = POL;
            	FrameLength = j;
            	break;
            case SELLECT_FRAME:
            	frame[j++] = EOT;
            	frame[j++] = STX;
            	frame[j++] = packet->address;
            	frame[j++] = packet->opcode;
            	for(i=0;i<packet->length;i++)
				{
					frame[j++] = packet->data[i];
				}
            	frame[j++] = ETX;
				sumLength = j - 2;
				frame[j++] = CheckSum(&frame[2],sumLength);
				FrameLength = j;
            	break;
            case ERROR_FRAME: // No error
            case NONE_FRAME:
            default:
                break;
        }
    }
    return FrameLength;
}

address_valid_t rs485_address_validate(uint8_t address_src, uint8_t address_dest)
{
    address_valid_t addr = {false, false , false};
#define RS485_DEVICE_TYPE_ADDRRESS_SHIFT       	(4)
#define RS485_DEVICE_TYPE_ADDRRESS_MASK        	(0x3)
#define RS485_DEVICE_TYPE_ADDRRESS(x)          	((x >> RS485_DEVICE_TYPE_ADDRRESS_SHIFT) & RS485_DEVICE_TYPE_ADDRRESS_MASK)
#define RS485_MIRROR_ADDRRESS_SHIFT      	   	(3)
#define RS485_MIRROR_ADDRRESS_MASK       	   	(0x1)
#define RS485_MIRROR_ADDRRESS(x)         		((x >> RS485_MIRROR_ADDRRESS_SHIFT) & RS485_MIRROR_ADDRRESS_MASK)
#define RS485_PHYSICAL_ADDRESS_SHIFT          	(0)
#define RS485_PHYSICAL_ADDRESS_MASK           	(0x7)
#define RS485_PHYSICAL_ADDRESS(x)             	((x >> RS485_PHYSICAL_ADDRESS_SHIFT) & RS485_PHYSICAL_ADDRESS_MASK)
    /*  addressing rule
    *   switch to address
    *  | BIT 7 | BIT 6 | BIT 5 | BIT 4 | BIT 3 | BIT 2 | BIT 1 | BIT 0 |
    *                  |   1   |   0   |  SW3  |  SW2  |  SW1  |  SW0  |
    *                  | device type   | mirror|       physical addr   |
    *
    */
    if(RS485_DEVICE_TYPE_ADDRRESS(address_src) == RS485_DEVICE_TYPE_ADDRRESS(address_dest)){
    	addr.device_type_valid = true;
    }
    if(RS485_PHYSICAL_ADDRESS(address_src) == RS485_PHYSICAL_ADDRESS(address_dest)){
    	addr.physical_addr_valid = true;
    }
    if(RS485_MIRROR_ADDRRESS(address_src) == RS485_MIRROR_ADDRRESS(address_dest)){
    	addr.is_reply = true;
    }
    return addr;
}
