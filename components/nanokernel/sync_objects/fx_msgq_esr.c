/** 
  ******************************************************************************
  *  @file   fx_msgq_esr.c
  *  @brief  Implementation of message queues for ESRs.
  *
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

#include FX_INTERFACE(FX_MSGQ_ESR)

FX_METADATA(({ implementation: [FX_MSGQ_ESR, V1] }))

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
//! Note that it locks scheduling for maximum N ESRs notification where N is
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
//! Try to send message into the queue.
//! @param msgq Message queue object to send to.
//! @param msg Message to be sent.
//! @return FX_MSGQ_OK in case of success, error code otherwise.
//!
int 
fx_msgq_send(fx_msgq_t* msgq, uintptr_t msg)
{
    int sent = false;
    fx_sched_state_t prev;
    fx_msgq_wait_attr_t attr;
    fx_sync_wait_block_t wb = FX_SYNC_WAIT_BLOCK_INITIALIZER(
        NULL, 
        NULL, 
        &attr
    );    
    lang_param_assert(msgq != NULL, FX_MSGQ_INVALID_PTR);
    lang_param_assert(fx_msgq_is_valid(msgq), FX_MSGQ_INVALID_OBJ); 

    attr.to_back = true;
    attr.buf = &msg;
    fx_sched_lock(&prev);
    sent = (int) fx_msgq_test_and_wait_send(&msgq->send_wtbl, &wb, false);
    fx_sched_unlock(prev);
    return sent ? FX_MSGQ_OK : FX_MSGQ_FULL;
}

//!
//! Attach ESR to message port.
//! Test port object for messages, if there are any, the message will be 
//! delivered to this ESR object. Otherwise the ESR will be attached to the port
//! and "wait" for a message, if "wait" option is true. If "wait" option is 
//! false - the function returns synchronously, without attaching to the port.
//! @param msgq Message queue.
//! @param wait Wait option. Function will not return synchronously if this 
//! option is true.
//! @return FX_MSGQ_OK in case of success, error code otherwise.
//!
int 
fx_msgq_listen(fx_msgq_t* msgq, const bool wait)
{
    lang_param_assert(msgq != NULL, FX_MSGQ_INVALID_PTR);
    lang_param_assert(fx_msgq_is_valid(msgq), FX_MSGQ_INVALID_OBJ); 

    return fx_esr_wait_msg(&msgq->recv_wtbl, wait) ? FX_MSGQ_OK :FX_MSGQ_NO_MSG;
}
