/** 
  ******************************************************************************
  *  @file   fx_msgq.c
  *  @brief  Implementation of message queues for threads.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(FX_MSGQ)

FX_METADATA(({ implementation: [FX_MSGQ, V1] }))

//!
//! Message queue constructor. 
//! @param [in] msgq Message queue to be initialized.
//! @param [in] buf Buffer to be used as queue storage. Buffer must be large 
//! enough to hold (items_max * sizeof(uintptr_t)) bytes.
//! @param [in] sz Maximum item count that may be stored in the queue.
//! @param [in] p Waiter notification policy.
//! @return FX_MSGQ_OK in case of success, error code otherwise.
//! @sa fx_msgq_deinit
//!
int 
fx_msgq_init(fx_msgq_t* msgq, uintptr_t* buf, unsigned sz, fx_sync_policy_t p)
{
    lang_param_assert(p < FX_SYNC_POLICY_MAX, FX_MSGQ_UNSUPPORTED_POLICY);
    lang_param_assert(msgq != NULL, FX_MSGQ_INVALID_PTR);
    lang_param_assert(buf != NULL, FX_MSGQ_INVALID_BUF);
    lang_param_assert(sz > 0, FX_MSGQ_INVALID_BUF);
    lang_param_assert(
        (((uintptr_t)buf) & (sizeof(uintptr_t) - 1)) == 0, 
        FX_MSGQ_INVALID_BUF
    );

    return fx_msgq_core_init(msgq, buf, sz, p);
}

//!
//! Message queue destructor. 
//! If there are active waiters in the queue, they will be deleased with 
//! DELETED status.
//! @param [in] msgq Message queue to be deinitialized.
//! @return FX_MSGQ_OK in case of success, error code otherwise.
//! @sa fx_msgq_deinit
//!
int 
fx_msgq_deinit(fx_msgq_t* msgq)
{
    lang_param_assert(msgq != NULL, FX_MSGQ_INVALID_PTR);
    lang_param_assert(fx_msgq_is_valid(msgq), FX_MSGQ_INVALID_OBJ); 

    return fx_msgq_core_deinit(msgq);
}

//!
//! Flushing the queue. 
//! Note that it locks scheduling for maximum N thread notification where N is
//! the queue length.
//! @param [in] msgq Message queue to be deinitialized.
//! @return FX_MSGQ_OK in case of success, error code otherwise.
//! @sa fx_msgq_deinit
//!
int 
fx_msgq_flush(fx_msgq_t* msgq)
{
    lang_param_assert(msgq != NULL, FX_MSGQ_INVALID_PTR);
    lang_param_assert(fx_msgq_is_valid(msgq), FX_MSGQ_INVALID_OBJ);

    return fx_msgq_core_flush(msgq);
}

//!
//! Send message into front of the queue. 
//! @param [in] msgq Message queue to send to.
//! @param [in] msg Message data. 
//! @param [in] cancel Event which cancels this wait operation if set
//! during waiting.
//! @return Result of wait operation.
//! @remark SPL = LOW.
//!
int 
fx_msgq_front_send(fx_msgq_t* msgq, uintptr_t msg, fx_event_t* cancel)
{
    fx_msgq_wait_attr_t attr;
    lang_param_assert(msgq != NULL, FX_MSGQ_INVALID_PTR);
    lang_param_assert(fx_msgq_is_valid(msgq), FX_MSGQ_INVALID_OBJ); 

    attr.to_back = false;
    attr.buf = &msg;
    return fx_thread_wait_object(&msgq->send_wtbl, &attr, cancel);
}

//!
//! Send message into front of the queue. 
//! @param [in] msgq Message queue to send to.
//! @param [in] msg Message data. 
//! @param [in] tout Timeout (in ticks) or FX_THREAD_INFINITE_TIMEOUT value.
//! @return Result of wait operation.
//! @remark SPL = LOW.
//!
int 
fx_msgq_front_timedsend(fx_msgq_t* msgq, uintptr_t msg, uint32_t tout)
{
    fx_msgq_wait_attr_t attr;
    lang_param_assert(msgq != NULL, FX_MSGQ_INVALID_PTR);
    lang_param_assert(fx_msgq_is_valid(msgq), FX_MSGQ_INVALID_OBJ); 

    attr.to_back = false;
    attr.buf = &msg;
    return fx_thread_timedwait_object(&msgq->send_wtbl, &attr, tout);
}

//!
//! Send message into the queue. 
//! @param [in] msgq Message queue to send to.
//! @param [in] msg Message data. 
//! @param [in] cancel_event Event which cancels this wait operation if set 
//! during waiting.
//! @return Result of wait operation.
//! @remark SPL = LOW.
//!
int 
fx_msgq_back_send(fx_msgq_t* msgq, uintptr_t msg, fx_event_t* cancel_event)
{
    int wait_res = FX_STATUS_OK;
    fx_msgq_wait_attr_t attr;
    lang_param_assert(msgq != NULL, FX_MSGQ_INVALID_PTR);
    lang_param_assert(fx_msgq_is_valid(msgq), FX_MSGQ_INVALID_OBJ); 

    attr.to_back = true;
    attr.buf = &msg;
    wait_res = fx_thread_wait_object(&msgq->send_wtbl, &attr, cancel_event);
    return wait_res; 
}

//!
//! Send message into queue. 
//! @param [in] msgq Message queue to send to.
//! @param [in] msg Message data. 
//! @param [in] tout Timeout (in ticks) or FX_THREAD_INFINITE_TIMEOUT value.
//! @return Result of wait operation.
//! @remark SPL = LOW.
//!
int 
fx_msgq_back_timedsend(fx_msgq_t* msgq, uintptr_t msg, uint32_t tout)
{
    int wait_res = FX_STATUS_OK;
    fx_msgq_wait_attr_t attr;
    lang_param_assert(msgq != NULL, FX_MSGQ_INVALID_PTR);
    lang_param_assert(fx_msgq_is_valid(msgq), FX_MSGQ_INVALID_OBJ); 

    attr.to_back = true;
    attr.buf = &msg;
    wait_res = fx_thread_timedwait_object(&msgq->send_wtbl, &attr, tout);
    return wait_res; 
}

//!
//! Receive message from the queue. If the queue is empty, calling thread is 
//! blocked until some other thread sends data to the queue or until cancel 
//! event is set. 
//! @param [in] msgq Message queue to receive from.
//! @param [out] msg Message buffer to be filled from queue. 
//! @param [in] cancel_event Event which cancels this wait operation if set 
//! during waiting.
//! @return Result of wait operation.
//! @remark SPL = LOW.
//!
int 
fx_msgq_receive(fx_msgq_t* msgq, uintptr_t* msg, fx_event_t* cancel_event)
{
    lang_param_assert(msgq != NULL, FX_MSGQ_INVALID_PTR);
    lang_param_assert(fx_msgq_is_valid(msgq), FX_MSGQ_INVALID_OBJ); 
    lang_param_assert(msg != NULL, FX_MSGQ_INVALID_BUF); 

    return fx_thread_wait_object(&msgq->recv_wtbl, msg, cancel_event);
}

//!
//! Receive message from queue. If the queue is empty, calling thread is blocked
//! until some other thread sends data to the queue or until specified timeout 
//! period is exceeded. 
//! @param [in] msgq Message queue to receive from.
//! @param [out] msg Message buffer to be filled from queue. 
//! @param [in] tout Timeout (in ticks) or FX_THREAD_INFINITE_TIMEOUT value.
//! @return Result of wait operation.
//! @remark SPL = LOW.
//!
int 
fx_msgq_timedreceive(fx_msgq_t* msgq, uintptr_t* msg, uint32_t tout)
{
    lang_param_assert(msgq != NULL, FX_MSGQ_INVALID_PTR);
    lang_param_assert(fx_msgq_is_valid(msgq), FX_MSGQ_INVALID_OBJ); 
    lang_param_assert(msg != NULL, FX_MSGQ_INVALID_BUF); 

    return fx_thread_timedwait_object(&msgq->recv_wtbl, msg, tout);
}
