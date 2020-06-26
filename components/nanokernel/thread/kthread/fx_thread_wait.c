/** 
  ******************************************************************************
  *  @file   fx_thread_wait.c
  *  @brief  Implementation of wait functions of threading subsystem.
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

#include FX_INTERFACE(FX_THREAD)

FX_METADATA(({ implementation: [FX_THREAD, V1] }))

//
// Calculation of wait status by statuses of two wait blocks used in wait funcs.
//
static int 
fx_thread_get_wait_status(fx_wait_status_t obj0, fx_wait_status_t obj1)
{
    //
    // If main object is signaled return SUCCESS status.
    //
    if (obj0 == FX_WAIT_SATISFIED)
    {
        return FX_STATUS_OK;      
    }

    //
    // If cancel object is signaled return CANCELLED status. It may be used to 
    // implement TIMEOUT and other cancel-semantics statuses.
    //
    if (obj1 == FX_WAIT_SATISFIED)
    {
        return FX_THREAD_WAIT_CANCELLED;
    }

    //
    // If some object deleted - return DELETED status.
    //
    if (obj0 == FX_WAIT_DELETED || obj1 == FX_WAIT_DELETED) 
    {
        return FX_THREAD_WAIT_DELETED;
    }
    
    //
    // Otherwise, if objects are neither signaled nor deleted it might happen 
    // only in case of wait abort due to APC delivery.
    //
    return FX_THREAD_WAIT_INTERRUPTED;
}

//!
//! This function is called as a result of notification (when wait is being 
//! satisfied).
//! SPL = SCHED_LEVEL.
//!
void 
fx_sync_waiter_notify(fx_sync_waiter_t* self)
{ 
    fx_thread_t* thread = lang_containing_record(self, fx_thread_t, waiter);

    fx_thread_lock(thread);
    fx_sched_item_resume(&thread->sched_item);
    thread->state = FX_THREAD_STATE_READY;
    fx_thread_unlock(thread);
    trace_thread_resume(&thread->trace_handle);
}

//!
//! Suspends current thread until waitable object is signaled. Wait operation 
//! may be cancelled by setting abort event.
//! @param [in] me Pointer to calling thread's object.
//! @param [in] object Waitable object to wait for.
//! @param [in] attr Additional attributes required for waitable object.
//! @param [in] abort_event Event object to be used as cancel event. Optional.
//! @return Result of wait operation.
//! @remark SPL = LOW. If both objects are signaled main object status will be 
//! returned.
//!
int 
fx_thread_wait_object_internal(
    fx_thread_t* me, 
    fx_sync_waitable_t* object, 
    void* attr, 
    fx_event_internal_t* abort_event)
{
    int wait_skip = FX_THREAD_WAIT_IN_PROGRESS;
    fx_sync_waitable_t* const cancel_waitable = abort_event ? 
        fx_internal_event_as_waitable(abort_event) : 
        NULL;
    fx_sched_state_t prev;
    unsigned int rolled_back_wb_num;
    fx_wait_status_t main_obj_status;
    fx_wait_status_t cancel_ev_status;

    fx_sync_wait_block_t wb[] = {
        FX_SYNC_WAIT_BLOCK_INITIALIZER(&me->waiter, object, attr),
        FX_SYNC_WAIT_BLOCK_INITIALIZER(&me->waiter, cancel_waitable, NULL)
    };

    fx_sync_waiter_prepare(&me->waiter, wb, sizeof(wb) / sizeof(wb[0]), 1);
    fx_sched_lock(&prev);

    do
    {
        if (object->test_wait(object, &(wb[0]), true))
        {
            wait_skip = FX_STATUS_OK;
            break;
        }

        if (!abort_event)
        {
            break;
        }

        if (fx_event_test_and_wait(cancel_waitable, &(wb[1]), true))
        {
            wait_skip = FX_THREAD_WAIT_CANCELLED;
            break;
        }
    }
    while (0);

    //
    // If no condition satisfied for wait cancelling.
    //
    if (wait_skip == FX_THREAD_WAIT_IN_PROGRESS)
    {
        //
        // Suspend thread. Because we're on SCHED_LEVEL level now, this thread 
        // continues execution.
        //
        fx_thread_lock(me);
        fx_sched_item_suspend(&me->sched_item);
        me->state = FX_THREAD_STATE_WAITING;
        fx_thread_unlock(me);

        //
        // Since waiter may receive notification callbacks (on SMP system) just 
        // after test&wait function is called, if notification is performed
        // on another CPU before "suspend", then notification will be missed 
        // (by specification resume function should not perform any actions if 
        // sched item is already in running state). In case when resume is 
        // missed (waiter already satisfied) we should do resume manually in 
        // order to prevent infinite sleep. If notification will be received 
        // between checking and resuming, then this resume will be skipped 
        // because thread will be already in running state.
        //
        if (fx_sync_is_waiter_satisfied(&me->waiter) || 
            fx_thread_apc_pending(&me->apcs))
        {
            fx_thread_lock(me);
            fx_sched_item_resume(&me->sched_item);
            me->state = FX_THREAD_STATE_READY;
            fx_thread_unlock(me);
        }

        //
        // Request rescheduling (wait preemption point).
        //
        fx_sched_unlock(prev);

        //
        // We come here when thread woken up by some reason.
        //
        fx_sched_lock(&prev);
        trace_thread_wakeup(&me->trace_handle);
    }

    //
    // Cancelling of wait block only has effect when block resides in 
    // waitable object's queue. 
    //
    rolled_back_wb_num = fx_sync_wait_rollback(&me->waiter);
    fx_sched_unlock(prev);
    main_obj_status = fx_sync_wait_block_get_status(&(wb[0]));
    cancel_ev_status = fx_sync_wait_block_get_status(&(wb[1]));

    //
    // Determine wait status to return. In case when wait was cancelled by 
    // incoming signal wait is already cancelled by signal handler, so, if we 
    // figure out that no wait blocks were cancelled it means that wait was 
    // interrupted.
    //
    return wait_skip == FX_THREAD_WAIT_IN_PROGRESS ? 
        rolled_back_wb_num ? 
            fx_thread_get_wait_status(main_obj_status, cancel_ev_status):
            FX_THREAD_WAIT_INTERRUPTED:
        wait_skip;
}

//!
//! Suspends current thread until waitable object is signaled and timeout is not
//! exceeded. 
//! @param [in] object Waitable object to wait for.
//! @param [in] attr Additional attributes, required by waitable object.
//! @param [in] timeout Timeout value. Use special value 
//! FX_THREAD_INFINITE_TIMEOUT for infinite.
//! @return Result of wait operation. 
//! @remark SPL = LOW. If timeout value is 0, then this function may be used to 
//! test waitable object: if object is nonsignaled then function immediately 
//! returns with FX_THREAD_WAIT_TIMEOUT status.
//!
int 
fx_thread_timedwait_object(
    fx_sync_waitable_t* object, 
    void* attr, 
    uint32_t timeout)
{
    fx_thread_t* const me = fx_thread_self();
    int error = FX_THREAD_WAIT_IN_PROGRESS;

    if (timeout == FX_THREAD_INFINITE_TIMEOUT)
    {
        error = fx_thread_wait_object_internal(me, object, attr, NULL);
    }
    else if (timeout == 0)
    {
        fx_sched_state_t prev;
        fx_sync_wait_block_t wb = FX_SYNC_WAIT_BLOCK_INITIALIZER(
            &me->waiter, 
            object, 
            attr
        );

        fx_sched_lock(&prev);
        error = object->test_wait(object, &wb, false) ? 
            FX_STATUS_OK : 
            FX_THREAD_WAIT_TIMEOUT;
        fx_sched_unlock(prev);
    }
    else
    {
        fx_event_internal_t* const cancel_ev = &me->timer_event;

        if (timeout < FX_TIMER_MAX_RELATIVE_TIMEOUT)
        {
            fx_event_internal_reset(cancel_ev);
            fx_timer_internal_set_rel(&me->timer, timeout, 0);
            error = fx_thread_wait_object_internal(me, object, attr, cancel_ev);
            fx_timer_internal_cancel(&me->timer);

            if (error == FX_THREAD_WAIT_CANCELLED)
            {
                error = FX_THREAD_WAIT_TIMEOUT;
                trace_thread_timeout(&me->trace_handle, timeout);
            }
        }
        else
        {
            error = FX_THREAD_INVALID_TIMEOUT;
        }
    }

    return error;
}

//!
//! Suspends current thread until waitable object is signaled. Wait operation 
//! may be cancelled by setting abort event.
//! @param [in] object Waitable object to wait for.
//! @param [in] attr Additional attributes required for waitable object.
//! @param [in] abort_event Event object to be used as cancel event. Optional.
//! @return Result of wait operation.
//! @remark SPL = LOW. If both objects are signaled main object status will be 
//! returned.
//!
int 
fx_thread_wait_object(
    fx_sync_waitable_t* object, 
    void* attr, 
    fx_event_t* abort_event)
{
    fx_event_internal_t* event = abort_event ? &abort_event->object : NULL;
    fx_thread_t* me = fx_thread_self();
    lang_param_assert(
        !event || fx_event_is_valid(abort_event), 
        FX_THREAD_INVALID_OBJ
    );

    return fx_thread_wait_object_internal(me, object, attr, event);
}

//!
//! Suspends current thread until event is signaled. Wait operation may be 
//! cancelled by setting abort event in active state.
//! @param [in] event Event object to wait for.
//! @param [in] abort_event Event object to be used as cancel event. Optional.
//! @return Result of wait operation. 
//! @remark SPL = LOW. If both objects are signaled main object status will be 
//! returned.
//!
int 
fx_thread_wait_event(fx_event_t* event, fx_event_t* abort_event)
{
    lang_param_assert(event != NULL, FX_THREAD_INVALID_OBJ);
    lang_param_assert(fx_event_is_valid(event), FX_THREAD_INVALID_OBJ);

    return fx_thread_wait_object(fx_event_as_waitable(event), NULL,abort_event);
}

//!
//! Suspends current thread until event is signaled and timeout is not exceeded. 
//! @param [in] event Event object to wait for.
//! @param [in] timeout Timeout value. Use special value 
//! FX_THREAD_INFINITE_TIMEOUT in order to implement infinite timeout.
//! @return Result of wait operation. 
//! @remark SPL = LOW.
//!
int 
fx_thread_timedwait_event(fx_event_t* event, uint32_t timeout)
{
    lang_param_assert(event != NULL, FX_THREAD_INVALID_OBJ);
    lang_param_assert(fx_event_is_valid(event), FX_THREAD_INVALID_OBJ);

    return fx_thread_timedwait_object(
        fx_event_as_waitable(event), 
        NULL, timeout
    );
}
