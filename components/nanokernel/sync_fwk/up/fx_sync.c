/** 
  ******************************************************************************
  *  @file   fx_sync.c
  *  @brief  Implementation of common synchronization layer.
  *  It provides simple wait/notify interface, which can be used to implement 
  *  sync primitives. This implementation supports uniprocessor systems only.
  *  Since on uniprocessor systems SPL is already guarantees proper 
  *  synchronization, waitable locking functions are unused.
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

#include FX_INTERFACE(FX_SYNC)
#include FX_INTERFACE(FX_DBG)
#include FX_INTERFACE(HAL_MP)

FX_METADATA(({ implementation: [FX_SYNC, UP_QUEUE] })) 

lang_static_assert(HAL_MP_CPU_MAX == 1);

//!
//! Waitable initializer.
//! @param [in,out] w Waitable to be initialized.
//! @param [in] lock Ignored.
//! @param [in] test_func Test function.
//!
void 
fx_sync_waitable_init(
    fx_sync_waitable_t* w, 
    void* lock, 
    bool (*test_func)(fx_sync_waitable_t*, fx_sync_wait_block_t*, const bool))
{
    rtl_queue_init(&w->wq);
    w->test_wait = test_func;
}

//!
//! Start of wait operation. Creates a link between waitable object and waiter.
//! @warning This function assumes object is already locked by caller.
//!
void 
_fx_sync_wait_start(fx_sync_waitable_t* waitable, fx_sync_wait_block_t* wb)
{
    wb->waitable = waitable; 
    rtl_enqueue(&waitable->wq, &wb->link);
}

//!
//! Gets wait block associated with a waitable.
//! @param [in] waitable Target waitable.
//! @param [in] policy Queue scheduling policy.
//! @return Wait block of waiter to be unlocked according to policy or NULL.
//! @warning  This function assumes object is already locked by caller.
//!
fx_sync_wait_block_t*   
_fx_sync_wait_block_get(fx_sync_waitable_t* waitable, fx_sync_policy_t policy)
{  
    fx_sync_wait_block_t* next = rtl_queue_entry(
        rtl_queue_first(&waitable->wq), 
        fx_sync_wait_block_t, 
        link
    );

    if (policy == FX_SYNC_POLICY_PRIO)
    {
        rtl_queue_t* head = &waitable->wq;
        rtl_queue_t* n = NULL;

        for (n = rtl_queue_first(head); n != head; n = rtl_queue_next(n)) 
        {
            fx_sync_wait_block_t* wb = fx_sync_queue_item_as_wb(n);
            fx_sync_waiter_t* waiter = wb->waiter;

            if (fx_sched_params_is_preempt(
                    waiter->sched_params, 
                    next->waiter->sched_params))
            {
                next = wb;
            }
        }
    }

    return next;
}

//!
//! Notification by wait block
//! @param [in] waitable Target waitable.
//! @param [in] reason Notifocation reason which must be provided by hi-level.
//! @param [in] wb Wait block of waiter to be released. 
//! @warning This function assumes object is already locked by caller.
//!
static inline void
_fx_sync_wait_notify_one(
    fx_sync_waitable_t* waitable, 
    fx_wait_status_t reason, 
    fx_sync_wait_block_t* wb)
{
    wb->u.status = reason;
    wb->waitable = NULL;
    fx_sync_waiter_notify(wb->waiter); 
}

//!
//! Waiter notification. 
//! Used by primitives to perform notification when object is signaled.
//! @param [in] waitable Target waitable.
//! @param [in] reason Notifocation reason which must be provided by hi-level.
//! @param [in] wb Wait block of waiter to be released (or NULL to notify all 
//! of them). 
//! @warning  This function assumes object is already locked by caller.
//!
void
_fx_sync_wait_notify(
    fx_sync_waitable_t* waitable, 
    fx_wait_status_t reason, 
    fx_sync_wait_block_t* wb)
{
    if (wb)
    {
        rtl_queue_remove(&wb->link);
        _fx_sync_wait_notify_one(waitable, reason, wb);
    }
    else
    {
        rtl_queue_linkage_t* q;
        while ((q = rtl_dequeue(&waitable->wq)) != NULL)
        {
            fx_sync_wait_block_t* wb_to_notify = fx_sync_queue_item_as_wb(q);
            _fx_sync_wait_notify_one(waitable, reason, wb_to_notify); 
        }
    }
} 

//!
//! Wait rollback. Cancels all wait blocks associated with specified waiter
//! @param [in,out] waiter Waiter, whose waits should be rolled back.
//! 
unsigned int               
fx_sync_wait_rollback(fx_sync_waiter_t* waiter)
{
    const unsigned int wb_num = waiter->wb_num;
    unsigned int i = 0;
    waiter->wb_num = 0;

    for (i = 0; i < wb_num; ++i)
    {
        fx_sync_wait_block_t* wb = &(waiter->wb[i]);       
        if (wb->waitable)
        {
            rtl_queue_remove(&wb->link);
            wb->u.status = FX_WAIT_CANCELLED;
            wb->waitable = NULL;
        }
    }

    return wb_num;
}
