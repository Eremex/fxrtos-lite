/** 
  ******************************************************************************
  *  @file   fx_msgq_core.c
  *  @brief  Implementation of core message queues.
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

#include FX_INTERFACE(FX_MSGQ_CORE)
#include FX_INTERFACE(HW_CPU)

FX_METADATA(({ implementation: [FX_MSGQ_CORE, V1] }))

//
// Helper function for actual data copying to the queue. 
// Should be called with object locked.
// 
static void
_fx_msgq_put_msg(
    fx_msgq_t* msgq, 
    const uintptr_t* const data_ptr, 
    const bool insert_to_back)
{
    if (insert_to_back)
    {
        msgq->buf[msgq->head] = *data_ptr;
        msgq->head = (msgq->head + 1) % msgq->items_max;
    }
    else
    {
        //
        // Since queue pointers are wrapped around, it is prohibited to
        // decrement pointers directly. Decrement should be performed as 
        // addition of (queue_size - 1) and then wrap.
        //
        msgq->tail = (msgq->tail + (msgq->items_max - 1)) % msgq->items_max;
        msgq->buf[msgq->tail] = *data_ptr;
    }
}

//!
//! Test and wait function for senders.
//! @param [in] object Waitable object to be tested.
//! @param [in] wb Wait block to be inserted into msgq's queue if it is 
//! nonsignaled.
//! @param [in] wait Wait option used to test object, if it is nonzero wait will
//! actually start in case when the queue is full, if it is zero - this function
//! just tests the object and return status.
//! @return true in case of object is signaled, false otherwise. 
//! @remark SPL = SCHED_LEVEL
//!
bool 
fx_msgq_test_and_wait_send(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait)
{
    fx_msgq_t* msgq = lang_containing_record(object, fx_msgq_t, send_wtbl);
    bool wait_satisfied = false;

    fx_sync_waitable_lock(object);

    //
    // If queue is full just insert wait block and start waiting.
    //
    if (msgq->items == msgq->items_max)
    {
        if (wait)
        {
            _fx_sync_wait_start(object, wb);
            trace_queue_send_block(&msgq->trace_handle);
        }
    }
    else
    {
        //
        // If queue is empty and there are receivers just copy data item 
        // directly into receiver's buffer and satisfy wait.
        //
        fx_msgq_wait_attr_t* attr = fx_sync_wait_block_get_attr(wb);

        if (msgq->items == 0 && _fx_sync_waitable_nonempty(&msgq->recv_wtbl))
        {
            fx_sync_wait_block_t* rcvr = _fx_sync_wait_block_get(
                &msgq->recv_wtbl, 
                msgq->policy
            );
            uintptr_t* rcvr_buf = fx_sync_wait_block_get_attr(rcvr);
            *rcvr_buf = *attr->buf;
            _fx_sync_wait_notify(&msgq->recv_wtbl, FX_WAIT_SATISFIED, rcvr);
            trace_queue_receive_forward(&msgq->trace_handle);
        }
        //
        // In all other cases copy value into queue.
        //
        else
        {
            _fx_msgq_put_msg(msgq, attr->buf, attr->to_back);
            ++msgq->items;
            trace_queue_send(&msgq->trace_handle, msgq->items);
        }

        wait_satisfied = true;
    }
    fx_sync_waitable_unlock(object); 
    return wait_satisfied; 
}

//!
//! Test and wait function for receivers.
//! @param [in] object Waitable object (encapsulated into the message queue) to 
//! be tested.
//! @param [in] wb Wait block to be inserted into msgq's queue if it is 
//! nonsignaled.
//! @param [in] wait Wait option used to test object, if it is nonzero wait will 
//! actually start in case when the queue is empty, if it is zero - this 
//! function just tests the object and return status.
//! @return true in case of object is signaled, false otherwise.
//! @remark SPL = SCHED_LEVEL
//!
static bool 
fx_msgq_test_and_wait_recv(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait)
{
    fx_msgq_t* msgq = lang_containing_record(object, fx_msgq_t, recv_wtbl);
    bool wait_satisfied = false;

    fx_sync_waitable_lock(object);

    //
    // If queue is empty just start waiting.
    //
    if (msgq->items == 0)
    {
        if (wait)
        {
            _fx_sync_wait_start(object, wb);
            trace_queue_receive_block(&msgq->trace_handle);
        }
    }
    else
    {
        uintptr_t* rcvr_buf = fx_sync_wait_block_get_attr(wb);
        *rcvr_buf = msgq->buf[msgq->tail];
        msgq->tail = (msgq->tail + 1) % msgq->items_max;
        --msgq->items;
        
        //
        // If queue was full and there was waiting senders, copy sender's 
        // message into queue and satisfy sender's wait operation.
        //
        if (msgq->items == msgq->items_max - 1 && 
            _fx_sync_waitable_nonempty(&msgq->send_wtbl))
        {
            fx_sync_wait_block_t* sndr = _fx_sync_wait_block_get(
                &msgq->send_wtbl, 
                msgq->policy
            );
            fx_msgq_wait_attr_t* attr = fx_sync_wait_block_get_attr(sndr);
            _fx_msgq_put_msg(msgq, attr->buf, attr->to_back);
            ++msgq->items;
            _fx_sync_wait_notify(&msgq->send_wtbl, FX_WAIT_SATISFIED, sndr);
            trace_queue_send_forward(&msgq->trace_handle);
        }
        else
        {
            trace_queue_receive(&msgq->trace_handle, msgq->items);
        }
        
        wait_satisfied = true;
    }
    
    fx_sync_waitable_unlock(object); 
    return wait_satisfied; 
}

//!
//! Message queue constructor. 
//! @param [in] msgq Message queue to be initialized.
//! @param [in] buf Buffer to be used as queue storage. Buffer must be large 
//! enough to hold (items_max * sizeof(uintptr_t)) bytes.
//! @param [in] items_max Maximum item count that may be stored in the queue.
//! @param [in] policy Waiter notification policy (FIFO or PRIO).
//! @return FX_STATUS_OK in case of success, error code otherwise.
//! @sa fx_msgq_deinit
//!
int 
fx_msgq_core_init(
    fx_msgq_t* msgq, 
    uintptr_t* buf, 
     unsigned int items_max, 
     fx_sync_policy_t policy)
{
    fx_spl_spinlock_init(&msgq->lock);
    fx_rtp_init(&msgq->rtp, FX_MSGQ_MAGIC);
    fx_sync_waitable_init(
        &msgq->send_wtbl, 
        &msgq->lock, 
        fx_msgq_test_and_wait_send
    );
    fx_sync_waitable_init(
        &msgq->recv_wtbl, 
        &msgq->lock, 
        fx_msgq_test_and_wait_recv
    );
    msgq->buf = buf;
    msgq->items_max = items_max;
    msgq->policy = policy;
    msgq->items = msgq->head = msgq->tail = 0;
    trace_queue_init(&msgq->trace_handle, items_max);
    return FX_STATUS_OK;
}

//!
//! Message queue destructor. All data contained in the queue will be flushed 
//! and all the waiters (both senders and receivers) will be released with 
//! FX_WAIT_DELETED status.
//! @param [in] msgq Message queue to be destroyed.
//! @return FX_STATUS_OK in case of success, error code otherwise.
//! @remark SPL = LOW.
//!
int 
fx_msgq_core_deinit(fx_msgq_t* msgq)
{
    fx_sched_state_t prev;
    fx_sched_lock(&prev);
    fx_rtp_deinit(&msgq->rtp);

    //
    // Both waitables share same lock, so, no matter which lock is acquired.
    //
    fx_sync_waitable_lock(&msgq->send_wtbl);
    _fx_sync_wait_notify(&msgq->send_wtbl, FX_WAIT_DELETED, NULL);
    _fx_sync_wait_notify(&msgq->recv_wtbl, FX_WAIT_DELETED, NULL);
    fx_sync_waitable_unlock(&msgq->send_wtbl);
    fx_sched_unlock(prev);
    trace_queue_deinit(&msgq->trace_handle, msgq->items);
    return FX_STATUS_OK;
}

//!
//! Flushing the message queue.
//! @param [in] msgq Message queue to be cleared.
//! @return FX_STATUS_OK in case of success, error code otherwise.
//! @remark SPL = LOW.
//!
int 
fx_msgq_core_flush(fx_msgq_t* msgq)
{  
    fx_sched_state_t prev;
    fx_sched_lock(&prev);
    fx_sync_waitable_lock(&msgq->send_wtbl);

    msgq->head = msgq->tail = msgq->items = 0;

    //
    // Waitable lock is released in notify function after notification of 
    // each waiter, so, queue state should be examined after each waiter is 
    // released.
    //
    while ( msgq->items < msgq->items_max && 
            _fx_sync_waitable_nonempty(&msgq->send_wtbl))
    {
        fx_sync_wait_block_t* sndr = _fx_sync_wait_block_get(
            &msgq->send_wtbl, 
            msgq->policy
        );
        fx_msgq_wait_attr_t* attr = fx_sync_wait_block_get_attr(sndr);
        _fx_msgq_put_msg(msgq, attr->buf, attr->to_back);
        ++msgq->items;
        _fx_sync_wait_notify(&msgq->send_wtbl, FX_WAIT_SATISFIED, sndr);
    }
    fx_sync_waitable_unlock(&msgq->send_wtbl);
    fx_sched_unlock(prev);
    return FX_STATUS_OK;
}
