/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "RingBuffer.h"

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/


/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/

#define BUFFER_FULL         E_OK
#define BUFFER_NOT_FULL     E_NOT_OK
#define BUFFER_EMPTY        E_OK
#define BUFFER_NOT_EMPTY    E_NOT_OK


/*==================================================================================================
*                                              ENUMS
==================================================================================================*/

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

Std_Return_Type RingBuffer_isFull(RingBufferManage_t *RingBuffer)
{
    return (RingBuffer->head == RingBuffer->tail) && (RingBuffer->n_elem == RingBuffer->size) ? BUFFER_FULL : BUFFER_NOT_FULL;
}
Std_Return_Type RingBuffer_isEmpty(RingBufferManage_t *RingBuffer)
{
    return (RingBuffer->head == RingBuffer->tail) && (RingBuffer->n_elem == 0) ? BUFFER_EMPTY : BUFFER_NOT_EMPTY;
}

Std_Return_Type RingBufferInit(RingBufferManage_t *RingBuffer, uint8_t *RingArray, uint16_t RingArrayLength, uint8_t QueueSize)
{
    Std_Return_Type retValue = E_OK;
    RingBuffer->mq_id        = RINGBUFFER_QUEUECREATE(QueueSize , sizeof(Cmd_Queue_Obj_t));
    RingBuffer->buff         = (uint8_t*)RingArray;
    RingBuffer->size         = RingArrayLength;
    RingBuffer->n_elem       = 0;
    RingBuffer->max_elem     = 0;
    RingBuffer->head         = 0;
    RingBuffer->tail         = 0;
    return retValue;
}
Std_Return_Type PutDataToBuffer(RingBufferManage_t *RingBuffer,uint8_t *pData ,uint16_t length, uint32_t timeout)
{
    uint16_t i;
    Cmd_Queue_Obj_t temp;
    OS_STATUS_T status = osOK;
    temp.tail = RingBuffer->head;
    for(i=0; i<length; i++)
    {
        if(BUFFER_FULL == RingBuffer_isFull(RingBuffer))
        {
            // buffer full and return to backup head
            RingBuffer->head = temp.tail;
            return E_NOT_OK;
        }else
        {
            RingBuffer->buff[RingBuffer->head] = pData[i];
            RingBuffer->head++;
            RingBuffer->head &= (RingBuffer->size - 1); // Avoid expensive modulo operation
        }
    }
    /* Mutex to avoid resource collision*/
    temp.head = RingBuffer->head;
    status = RINGBUFFER_QUEUEPUT(RingBuffer->mq_id, (void*)&temp, timeout);
    if(status != osOK)
    {
        RingBuffer->head = temp.tail;
        return E_NOT_OK;
    }
    RingBuffer->n_elem = RingBuffer->n_elem + length;
    if(RingBuffer->n_elem > RingBuffer->max_elem)
    {
        /* update new maximum elements thats use just for management*/
        RingBuffer->max_elem = RingBuffer->n_elem;
    }
    return E_OK;
    
}
Std_Return_Type GetDataFromBuffer(RingBufferManage_t *RingBuffer,uint8_t *pData ,uint16_t *length, uint32_t timeout)
{
	OS_STATUS_T CmdStatus;
    Std_Return_Type RetValue = E_NOT_OK;
    Cmd_Queue_Obj_t GetDataInfo;
    *length = 0;
    CmdStatus = RINGBUFFER_QUEUEGET(RingBuffer->mq_id, &GetDataInfo, timeout);
    if(osOK == CmdStatus) 
    {
        if(GetDataInfo.tail < GetDataInfo.head)
        {
            *length = GetDataInfo.head - GetDataInfo.tail;
            memcpy(pData, &(RingBuffer->buff[GetDataInfo.tail]),*length);
        }else if(GetDataInfo.tail > GetDataInfo.head)
        {
            *length = RingBuffer->size - GetDataInfo.tail;
            /* Copy data from Tail to the end of buffer */
            memcpy(pData, &(RingBuffer->buff[GetDataInfo.tail]),*length);
            if(0 != GetDataInfo.head)
            {
                /* Copy the rest of data */
                memcpy(&pData[*length], &(RingBuffer->buff[0]),GetDataInfo.head);
                *length += GetDataInfo.head;
            }
        }
        RingBuffer->tail = GetDataInfo.head;
        RingBuffer->n_elem = RingBuffer->n_elem - *length;
        RetValue = E_OK;
    }
    return RetValue;
}

#if( OS_TYPE == FREERTOS)
Std_Return_Type PutDataToBufferISR(RingBufferManage_t *RingBuffer,uint8_t *pData ,uint16_t length, BaseType_t *pxHigherPriorityTaskWoken)
{
    uint16_t i;
    Cmd_Queue_Obj_t temp;
    OS_STATUS_T status = osOK;
    temp.tail = RingBuffer->head;
    for(i=0; i<length; i++)
    {
        if(BUFFER_FULL == RingBuffer_isFull(RingBuffer))
        {
            // buffer full and return to backup head
            RingBuffer->head = temp.tail;
            return E_NOT_OK;
        }else
        {
            RingBuffer->buff[RingBuffer->head] = pData[i];
            RingBuffer->head++;
            RingBuffer->head &= (RingBuffer->size - 1);
        }
    }
    /* Mutex to avoid resource collision*/
    temp.head = RingBuffer->head;
    status = RINGBUFFER_QUEUEPUT_ISR(RingBuffer->mq_id, (void*)&temp, pxHigherPriorityTaskWoken);
    if(status != osOK)
    {
        RingBuffer->head = temp.tail;
        return E_NOT_OK;
    }
    RingBuffer->n_elem = RingBuffer->n_elem + length;
    if(RingBuffer->n_elem > RingBuffer->max_elem)
    {
        /* update new maximum elements thats use just for management*/
        RingBuffer->max_elem = RingBuffer->n_elem;
    }
    return E_OK;

}

Std_Return_Type GetDataFromBufferISR(RingBufferManage_t *RingBuffer,uint8_t *pData ,uint16_t *length, BaseType_t *pxHigherPriorityTaskWoken)
{
	OS_STATUS_T CmdStatus;
    Std_Return_Type RetValue = E_NOT_OK;
    Cmd_Queue_Obj_t GetDataInfo;
    *length = 0;
    CmdStatus = RINGBUFFER_QUEUEGET_ISR(RingBuffer->mq_id, (void*)&GetDataInfo, pxHigherPriorityTaskWoken);
    if(osOK == CmdStatus)
    {
        if(GetDataInfo.tail < GetDataInfo.head)
        {
            *length = GetDataInfo.head - GetDataInfo.tail;
            memcpy(pData, &(RingBuffer->buff[GetDataInfo.tail]),*length);
        }else if(GetDataInfo.tail > GetDataInfo.head)
        {
            *length = RingBuffer->size - GetDataInfo.tail;
            /* Copy data from Tail to the end of buffer */
            memcpy(pData, &(RingBuffer->buff[GetDataInfo.tail]),*length);
            if(0 != GetDataInfo.head)
            {
                /* Copy the rest of data */
                memcpy(&pData[*length], &(RingBuffer->buff[0]),GetDataInfo.head);
                *length += GetDataInfo.head;
            }
        }
        RingBuffer->tail = GetDataInfo.head;
        RingBuffer->n_elem = RingBuffer->n_elem - *length;
        RetValue = E_OK;
    }
    return RetValue;
}
#endif
