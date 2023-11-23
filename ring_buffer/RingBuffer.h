#ifndef RINGBUFFER_H
#define RINGBUFFER_H

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "Standard.h"

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
///**
// * @brief Std_Return_Type
// */
//typedef enum
//{
//	E_OK = 0U,
//	E_NOT_OK = 1U
//} Std_Return_Type;

#define FREERTOS	1
#define KEILRTOS	2
#define OS_TYPE		FREERTOS
#if( OS_TYPE == FREERTOS)
#define QUEUE_HANDLE_T											QueueHandle_t
#define OS_STATUS_T												BaseType_t
#define RINGBUFFER_QUEUECREATE(msg_count, msg_size)             xQueueCreate(msg_count, msg_size)
#define RINGBUFFER_QUEUEPUT(queue_id, data_ptr, timeout)        xQueueSend(queue_id, (void*)data_ptr, timeout)
#define RINGBUFFER_QUEUEGET(queue_id, data_ptr, timeout)        xQueueReceive(queue_id, (void*)data_ptr, timeout)
#define RINGBUFFER_QUEUEPUT_ISR(queue_id, data_ptr, HigherPriorityTaskWoken)    xQueueSendFromISR(queue_id, (void*)data_ptr, (BaseType_t *)HigherPriorityTaskWoken)
#define RINGBUFFER_QUEUEGET_ISR(queue_id, data_ptr, HigherPriorityTaskWoken)    xQueueReceiveFromISR(queue_id, (void*)data_ptr, (BaseType_t *)HigherPriorityTaskWoken)
#elif ( OS_TYPE == KEILRTOS)
#define QUEUE_HANDLE_T											osMessageQueueId_t
#define OS_STATUS_T												osStatus_t
#endif
/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                              ENUMS
==================================================================================================*/
typedef struct{
    uint8_t  *buff;
    uint16_t size;
    uint16_t tail;
    uint16_t head;
    uint16_t n_elem;
    uint16_t max_elem;
    QUEUE_HANDLE_T  mq_id;
}RingBufferManage_t;


typedef struct{
    uint16_t head;
    uint16_t tail;
}Cmd_Queue_Obj_t;

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
Std_Return_Type RingBuffer_isFull(RingBufferManage_t *RingBuffer);
Std_Return_Type RingBuffer_isEmpty(RingBufferManage_t *RingBuffer);
Std_Return_Type RingBufferInit(RingBufferManage_t *RingBuffer, uint8_t *RingArray, uint16_t RingArrayLength, uint8_t QueueSize);
Std_Return_Type PutDataToBuffer(RingBufferManage_t *RingBuffer,uint8_t *pData ,uint16_t length, uint32_t timeout);
Std_Return_Type GetDataFromBuffer(RingBufferManage_t *RingBuffer,uint8_t *pData,uint16_t *length, uint32_t timeout);
#if( OS_TYPE == FREERTOS)
Std_Return_Type PutDataToBufferISR(RingBufferManage_t *RingBuffer,uint8_t *pData ,uint16_t length, BaseType_t *pxHigherPriorityTaskWoken);
Std_Return_Type GetDataFromBufferISR(RingBufferManage_t *RingBuffer,uint8_t *pData,uint16_t *length, BaseType_t *pxHigherPriorityTaskWoken);
#endif
#endif /* RINGBUFFER_H */
