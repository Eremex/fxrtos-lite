/** 
  ******************************************************************************
  *  @file   fx_ev_flags.c
  *  @brief  Implementation of event flags.
  *  Note that event flags is not meet hard real-time requirements,
  *  at every change of the flags state all waiting threads should be 
  *  analyzed resulting in O(n) scheduling and interrupt latencies
  *  (in order to provide multiprocessor consistency analyzing is 
  *  performed within critical section guarded by spinlock).
  *  So, number of waiting threads should be reasonable and bounded,
  *  to make the system deterministic.
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

#include FX_INTERFACE(FX_EV_FLAGS)
#include FX_INTERFACE(HW_CPU)

FX_METADATA(({ implementation: [FX_EV_FLAGS, V1] }))

#define fx_ev_flags_is_valid(e) (fx_rtp_check((&((e)->rtp)), FX_EV_FLAGS_MAGIC))

//!
//! Additional attributes associated with each wait operation.
//!
typedef struct
{
    unsigned int type;    //!< Options whether the flags should be cleared, etc.
    uint_fast32_t flags;  //!< Requested flags to wait.
    uint_fast32_t prev;   //!< Event flags state which unlocks the wait.
}
fx_ev_flags_attr_t;

#define wait_all(type) (((type) & FX_EV_FLAGS_AND) != 0)
#define wait_any(type) (((type) & FX_EV_FLAGS_AND) == 0)

//
// Internal function used to check event flags state and determine should the 
// waiter put itself into waiters queue or the request may be satisfied 
// immediately.
//
static bool 
fx_evf_test_and_wait(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait)
{
    fx_ev_flags_attr_t* const attr = fx_sync_wait_block_get_attr(wb);
    bool satisfied = false;
    fx_ev_flags_t* const evf = lang_containing_record(
        object, 
        fx_ev_flags_t, 
        waitable
    );

    fx_sync_waitable_lock(object);
    {
        const uint_fast32_t intersect = attr->flags & evf->flags;

        if ((wait_all(attr->type) && intersect == attr->flags) || 
            (wait_any(attr->type) && intersect != 0))
        {
            attr->prev = evf->flags;
            if (attr->type & FX_EV_FLAGS_CLEAR)
            {
                evf->flags &= ~attr->flags;
            }
            satisfied = true;
        }
        else if (wait)
        {
            _fx_sync_wait_start(object, wb);
        }
    }
    fx_sync_waitable_unlock(object);

    return satisfied;
}

//!
//! Event flags initialization.
//! @param [in] evf Pointer to user-allocated event flags object.
//! @return FX_EV_FLAGS_OK in case of success, error code otherwise.
//!
int 
fx_ev_flags_init(fx_ev_flags_t* evf)
{
    lang_param_assert(evf != NULL, FX_EV_FLAGS_INVALID_PTR);

    fx_rtp_init(&evf->rtp, FX_EV_FLAGS_MAGIC);
    fx_spl_spinlock_init(&evf->lock);

    //
    // Both internal wait queues share single lock!
    //
    fx_sync_waitable_init(&evf->waitable, &evf->lock, fx_evf_test_and_wait);
    rtl_queue_init(&evf->temp);
    evf->flags = 0;
    return FX_EV_FLAGS_OK;
}

//!
//! Event flags destructor.
//! @param [in] evf Pointer to event flags object to be deleted.
//! @return FX_EV_FLAGS_OK in case of success, error code otherwise.
//!
int 
fx_ev_flags_deinit(fx_ev_flags_t* evf)
{
    fx_sched_state_t prev;
    lang_param_assert(evf != NULL, FX_EV_FLAGS_INVALID_PTR);
    lang_param_assert(fx_ev_flags_is_valid(evf), FX_EV_FLAGS_INVALID_OBJ);

    fx_sched_lock(&prev);
    fx_rtp_deinit(&evf->rtp);
    fx_sync_waitable_lock(&evf->waitable);
    _fx_sync_wait_notify(&evf->waitable, FX_WAIT_DELETED, NULL);
    fx_sync_waitable_unlock(&evf->waitable);
    fx_sched_unlock(prev);
    return FX_EV_FLAGS_OK;
}

//!
//! Setting or clearing specified event flags.
//! @param [in] evf Pointer to event flags object.
//! @param [in] flags Requested flags to be changed.
//! @param [in] set Set specified flags to 1 if true, reset specified flags to 
//! 0 if false.
//! @return FX_EV_FLAGS_OK in case of success, error code otherwise.
//!
int  
fx_ev_flags_set(fx_ev_flags_t* evf, uint_fast32_t flags, bool set)
{
    fx_sched_state_t prev;
    lang_param_assert(evf != NULL, FX_EV_FLAGS_INVALID_PTR);
    lang_param_assert(fx_ev_flags_is_valid(evf), FX_EV_FLAGS_INVALID_OBJ);
    lang_param_assert(flags != 0, FX_EV_FLAGS_INVALID_FLAGS);

    fx_sched_lock(&prev);
    fx_sync_waitable_lock(&evf->waitable);

    if (set)
    {
        rtl_queue_t* const head = fx_sync_waitable_as_queue(&evf->waitable);
        const rtl_queue_t* n = rtl_queue_first(head);
        uint_fast32_t flags_to_clear = 0;
        evf->flags |= flags;

        while (n != head) 
        {
            fx_sync_wait_block_t* const wb = fx_sync_queue_item_as_wb(n);
            fx_ev_flags_attr_t* const attr = fx_sync_wait_block_get_attr(wb);
            const uint_fast32_t intersect = attr->flags & evf->flags;
            n = rtl_queue_next(n);

            //
            // If wait condition is satisfied.
            //
            if ((wait_all(attr->type) && intersect == attr->flags) || 
                (wait_any(attr->type) && intersect != 0))
            {
                //
                // If waiter consume flags, mark them to be reset.
                //
                if (attr->type & FX_EV_FLAGS_CLEAR)
                {
                    flags_to_clear |= attr->flags;
                }

                //
                // Provide flags state which satisfies the wait.
                //
                attr->prev = evf->flags;
                
                //
                // Move wait block to temporary queue holding all items to be 
                // resumed.
                //
                rtl_queue_remove(fx_sync_wb_as_queue_item(wb));
                rtl_queue_insert(&evf->temp, fx_sync_wb_as_queue_item(wb));
            }
        }

        //
        // Now temporary queue holds all the items should be resumed and we have
        // gathered all flags to be consumed/cleared. 
        // Note: notification process is performed step by step with spinlock 
        // releasing. since temporary queue is common for whole primitive it is 
        // possible that another CPUs are process it simultaneously. In any way,
        // items which reside in this waitable should be notified, no matter 
        // which CPU will make it. Another issue is wait block status and 
        // cancellation. Synchronization framework requires that wait block 
        // contained in any queue MUST have state "in_progress", it is only way 
        // when cancellation works properly. When wait is canceled when WB was 
        // in temporary queue before it is notified, wait function will get 
        // CANCELLED status rather than SUCCESS. It is the reason why successful
        // state of wait operation is determined by attributes, not by actual 
        // wait block status.
        //
        evf->flags &= ~flags_to_clear;
        
        while (!rtl_queue_empty(&evf->temp))
        {
            const rtl_queue_t* n = rtl_queue_first(&evf->temp);
            fx_sync_wait_block_t* const wb = fx_sync_queue_item_as_wb(n);
            _fx_sync_wait_notify(&evf->waitable, FX_WAIT_SATISFIED, wb);
        }
    }
    else
    {
        evf->flags &= ~flags;
    }
    fx_sync_waitable_unlock(&evf->waitable);
    fx_sched_unlock(prev);
    return FX_EV_FLAGS_OK;
}

//!
//! Waiting for specified flags to be set (with optional cancellation event).
//! @param [in] evf Pointer to event flags object.
//! @param [in] req_flags Requested flags to wait for.
//! @param [in] option Wait options, valid flags are 
//! FX_EV_FLAGS_OR/FX_EV_FLAGS_AND and FX_EV_FLAGS_CLEAR.
//! for example: FX_EV_FLAGS_AND | FX_EV_FLAGS_CLEAR waits all flags to be set 
//! and clears them after wait operation is satisfied.
//! @param [in] state Optional pointer to flags state which satisfies wait 
//! operation.
//! @param [in] cancel_ev Cancellation event.
//! @return FX_EV_FLAGS_OK in case of success, error code otherwise.
//!
int 
fx_ev_flags_wait(
    fx_ev_flags_t* evf, 
    const uint_fast32_t req_flags, 
    const unsigned int option, 
    uint_fast32_t* state,
    fx_event_t* cancel_ev)
{
    fx_ev_flags_attr_t attr;
    int error = FX_EV_FLAGS_OK;
    lang_param_assert(evf != NULL, FX_EV_FLAGS_INVALID_PTR);
    lang_param_assert(fx_ev_flags_is_valid(evf), FX_EV_FLAGS_INVALID_OBJ);
    lang_param_assert(req_flags != 0, FX_EV_FLAGS_INVALID_FLAGS);
    lang_param_assert((option & ~0x3) == 0, FX_EV_FLAGS_INVALID_OPTIONS);

    attr.type = option;
    attr.flags = req_flags;
    attr.prev = 0;
    error = fx_thread_wait_object(&evf->waitable, &attr, cancel_ev);

    //
    // If attributes were modified, it means that wait block representing 
    // current wait operation has  been examined and event flag state might be
    // modified. On the other hand, in multiprocessor system, wait operation may 
    // be aborted during notification process: determining which threads have to 
    // be released (and applying side effects to flags) is atomic, but
    // notification process is performed step-by-step and may be cancelled
    // before appropriate wait block is notified. Wait block contains
    // cancellation status as error code in this case. So, if attributes were
    // modified, operation should be considered as successful, regardless of
    // status returned by fx_thread_wait_object. If attributes were not 
    // modified, then error code provided by fx_thread_wait_object should be 
    // returned.
    //
    if (attr.prev)
    {
        error = FX_EV_FLAGS_OK;
        if (state)
        {
            *state = attr.prev;
        }
    }
    return error;
}

//!
//! Waiting for specified flags to be set with timeout.
//! @param [in] evf Pointer to event flags object.
//! @param [in] req_flags Requested flags to wait for.
//! @param [in] option Wait options, valid flags are 
//! FX_EV_FLAGS_OR/FX_EV_FLAGS_AND and FX_EV_FLAGS_CLEAR.
//! for example: FX_EV_FLAGS_AND | FX_EV_FLAGS_CLEAR waits all flags to be set 
//! and clears them after wait operation is satisfied.
//! @param [in] state Optional pointer to flags state which satisfies wait call.
//! @param [in] tout Relative timeout. Use FX_THREAD_INFINITE_TIMEOUT for 
//! infinite timeout.
//! @return FX_EV_FLAGS_OK in case of success, error code otherwise.
//!
int 
fx_ev_flags_timedwait(
    fx_ev_flags_t* evf, 
    const uint_fast32_t req_flags, 
    const unsigned int option, 
    uint_fast32_t* state, 
    uint32_t tout)
{
    fx_ev_flags_attr_t attr;
    int error = FX_EV_FLAGS_OK;
    lang_param_assert(evf != NULL, FX_EV_FLAGS_INVALID_PTR);
    lang_param_assert(fx_ev_flags_is_valid(evf), FX_EV_FLAGS_INVALID_OBJ);
    lang_param_assert(req_flags != 0, FX_EV_FLAGS_INVALID_FLAGS);
    lang_param_assert((option & ~0x3) == 0, FX_EV_FLAGS_INVALID_OPTIONS);

    attr.type = option;
    attr.flags = req_flags;
    attr.prev = 0;
    error = fx_thread_timedwait_object(&evf->waitable, &attr, tout);

    if (attr.prev)
    {
        error = FX_EV_FLAGS_OK;
        if (state)
        {
            *state = attr.prev;
        }      
    }
    return error;
}
