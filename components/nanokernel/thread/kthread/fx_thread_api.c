/**
  ******************************************************************************
  *  @file   fx_thread_api.c
  *  @brief  Implementation of application interface of the threading subsystem.
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
#include FX_INTERFACE(HAL_MP)
#include FX_INTERFACE(FX_THREAD_TIMESLICE)

FX_METADATA(({ implementation: [FX_THREAD, V1] }))

#define fx_thread_is_valid(thr) (fx_rtp_check((&((thr)->rtp)), FX_THREAD_MAGIC))

int fx_thread_wait_object_internal(
    fx_thread_t* self, 
    fx_sync_waitable_t* object, 
    void* attr, 
    fx_event_internal_t* cancel
);

//!
//! Initialize new thread.
//! @param [in] parent Parent process.
//! @param [in, out] thread User-allocated thread object to be initialized.
//! @param [in] func Entry point of new thread.
//! @param [in] arg Argument for thread function.
//! @param [in] priority Thread priority. Less number means higher priority.
//! @param [in] stack User-allocated stack for the new thread. Must not be NULL.
//! @param [in] stack_sz Thread's stack size. Must not be 0.
//! @param [in] create_suspended If nonzero then thread will be created in 
//! suspended state.
//! @return FX_STATUS_OK if succeeded, error code otherwise.
//!
int
fx_thread_init_ex(
    fx_process_t* parent, 
    fx_thread_t* thread, void (*func)(void*), void* arg,
    unsigned int priority, 
    void* stack, size_t stack_sz,
    bool create_suspended)
{
    const uintptr_t kstack = (uintptr_t)((uint8_t*)stack + stack_sz);
    fx_sched_state_t prev;
    fx_sched_params_t params;
    int (*tout_callback)(void*) = (int(*)(void*)) fx_event_internal_set;
    fx_sched_affinity_t parent_affinity = 0;

    lang_param_assert(thread != NULL, FX_THREAD_INVALID_PTR);
    lang_param_assert(func != NULL, FX_THREAD_INVALID_ENTRY);
    lang_param_assert(stack_sz != 0, FX_THREAD_NO_STACK);
    lang_param_assert(stack != NULL, FX_THREAD_NO_STACK);
    lang_param_assert(priority < FX_SCHED_ALG_PRIO_IDLE,FX_THREAD_INVALID_PRIO);

    thread->state = FX_THREAD_STATE_SUSPENDED;
    thread->parent = parent;
    thread->is_terminating = false;
    thread->timeslice = 0;
    fx_sched_params_init_prio(&params, priority);
    fx_sched_item_init(
        &thread->sched_item, 
        FX_SCHED_PARAMS_INIT_SPECIFIED, 
        &params
    );
    fx_sched_get_affinity(&(fx_thread_self()->sched_item), &parent_affinity);
    fx_sched_set_affinity(&thread->sched_item, &parent_affinity, false);
    fx_rtp_init(&thread->rtp, FX_THREAD_MAGIC);
    fx_sync_waiter_init(&thread->waiter, fx_thread_as_sched_params(thread));
    fx_thread_apc_target_init(&thread->apcs);
    fx_thread_cleanup_init_target(&thread->cleanup);
    fx_timer_internal_init(&thread->timer, tout_callback, &thread->timer_event);
    fx_event_internal_init(&thread->completion, false);
    fx_event_internal_init(&thread->timer_event, false);
    fx_stackovf_init(&thread->stk_info, stack, stack_sz);
    fx_spl_spinlock_init(&thread->state_lock);
    trace_thread_init(
        &thread->trace_handle, 
        fx_sched_item_as_number(&thread->sched_item)
    );
    hal_context_ker_create(
        &thread->hw_context, 
        kstack, 
        (uintptr_t)func, 
        (uintptr_t)arg
    );

    fx_sched_lock(&prev);
    fx_sched_item_add(&thread->sched_item);

    //
    // Scheduler are items always initialized as suspended, resume thread if 
    // create_suspended option is not specified.
    // N.B. thread state is protected by the spinlock, but for new threads it is
    // accessed without the lock since it is not allowed to use threads which 
    // are not completely initialized.
    //
    if (!create_suspended)
    {
        thread->state = FX_THREAD_STATE_READY;
        fx_sched_item_resume(&thread->sched_item);
    }
    fx_sched_unlock(prev);
    return FX_STATUS_OK;
}

//!
//! Thread destructor.
//! Thread destructor should not be called while thread is active.
//! @param [in] thread Thread to be destructed.
//! @return FX_STATUS_OK if succeeded, error code otherwise.
//!
int
fx_thread_deinit(fx_thread_t* thread)
{
    fx_sched_state_t state;
    lang_param_assert(thread != NULL, FX_THREAD_INVALID_PTR);
    lang_param_assert(fx_thread_is_valid(thread), FX_THREAD_INVALID_OBJ);

    fx_sched_lock(&state);
    fx_rtp_deinit(&thread->rtp);
    hal_context_ker_delete(&thread->hw_context);
    trace_thread_deinit(
        &thread->trace_handle, 
        fx_sched_params_as_number(fx_thread_as_sched_params(thread))
    );
    fx_sched_unlock(state);
    return FX_STATUS_OK;
}

//!
//! This function is used to cause a thread to exit.
//! Thread cannot protect itself from termination. Use this function only in 
//! exceptional cases, because thread termination is a potentially dangerous 
//! operation. Any resources owned by thread will not be released (mutexes, 
//! critical sections and so on).
//! @param [in] thread Thread to be terminated.
//! @return FX_STATUS_OK if succeeded, error code otherwise.
//!
int
fx_thread_terminate(fx_thread_t* thread)
{
    fx_sched_state_t state;
    lang_param_assert(thread != NULL, FX_THREAD_INVALID_PTR);
    lang_param_assert(fx_thread_is_valid(thread), FX_THREAD_INVALID_OBJ);
    
    fx_sched_lock(&state);
    fx_thread_lock(thread);
      
    if (!thread->is_terminating)
    {
        thread->is_terminating = true;

        //
        // Send termination APC. 
        // Termination will be performed in the APC handler. If APC has beed 
        // successfully inserted into the queue then the thread must be made 
        // runnable.
        //
        if (fx_thread_apc_insert_internal(&thread->apcs, 0,&thread->completion))
        {
            const unsigned thread_cpu = fx_sched_get_cpu(&thread->sched_item);
            thread->state = FX_THREAD_STATE_READY;
            fx_sched_item_resume(&thread->sched_item);

            //
            // Request rescheduling in order to force APC delivery in case when 
            // target thread was in ready state and resuming took no effect.
            //
            hal_mp_request_ipi(thread_cpu, SPL_DISPATCH);
        }
    }
    
    fx_thread_unlock(thread);
    fx_sched_unlock(state);
    return FX_STATUS_OK;
}

//!
//! Stops execution of current thread.
//! @param [in] me Caller thread.
//! @param [in] exit_code Thread exit code.
//! SPL = SCHED_LEVEL, APC masked
//!
void
fx_thread_exit_sync(fx_thread_t* me)
{
    //
    // Mark thread as finished. Scheduler sets completion event for threads in 
    // this state when moves on next thread in ready state.
    //  
    fx_thread_lock(me);
    me->state = FX_THREAD_STATE_COMPLETED;
    
    //
    // Remove itself from scheduler data structures.
    // N.B. Removing from scheduler also cancels any asynchronous activity 
    // related to thread.
    //
    fx_sched_item_remove(&me->sched_item);
    fx_thread_unlock(me);
}

//!
//! Stops execution of current thread.
//! @param [in] exit_code Thread exit code.
//!
void
fx_thread_exit(void)
{
    fx_sched_state_t prev;
    fx_thread_t* const me = fx_thread_self();
    
    //
    // Disable APC. Now thread cannot be made runnable by incoming APCs.
    //
    (void) fx_thread_apc_set_mask(true);

    //
    // Call cleanup handlers if any. 
    // N.B. Thread will not die until all waits in these handlers will be done.
    //
    fx_thread_cleanup_handle(fx_thread_as_cleanup_context(me));
    fx_sched_lock(&prev);
    fx_thread_exit_sync(me);
    fx_sched_unlock(prev);
}

//!
//! Suspends calling thread.
//! @return FX_STATUS_OK in case of success, error code otherwise.
//!
int
fx_thread_suspend(void)
{
    fx_thread_t* me;
    fx_sched_state_t prev;
    fx_sched_lock(&prev);
    me = fx_thread_self();
    fx_thread_lock(me);
    
    //
    // Thread suspension prevents APCs to be delivered, so, check the APCs 
    // presence and skip the suspension in case when some APCs are pending.
    // From user's perspective it is seen as wait/suspend abort due to APC 
    // delivery.
    //
    if (!fx_thread_apc_pending(&me->apcs))
    {
        fx_sched_item_suspend(&me->sched_item);
        me->state = FX_THREAD_STATE_SUSPENDED;
    }

    fx_thread_unlock(me);
    trace_thread_suspend(&me->trace_handle);
    fx_sched_unlock(prev);
    return FX_STATUS_OK;
}

//!
//! Resumes specified thread.
//! @param [in] thread Thread to be resumed.
//! @return FX_STATUS_OK in case of success, error code otherwise.
//!
int
fx_thread_resume(fx_thread_t* thread)
{
    fx_sched_state_t prev;
    lang_param_assert(thread != NULL, FX_THREAD_INVALID_PTR);
    lang_param_assert(fx_thread_is_valid(thread), FX_THREAD_INVALID_OBJ);

    fx_sched_lock(&prev);
    fx_thread_lock(thread);
    
    //
    // Resume may be called for suspended threads only. In case when the 
    // thread is either ready or completed no action should be performed.
    //
    if (thread->state == FX_THREAD_STATE_SUSPENDED)
    {
        fx_sched_item_resume(&thread->sched_item);
        thread->state = FX_THREAD_STATE_READY;
    }
    fx_thread_unlock(thread);
    trace_thread_resume(&thread->trace_handle);
    fx_sched_unlock(prev);
    return FX_STATUS_OK;
}

//!
//! Stops thread execution for specified time interval.
//! @param [in] ticks Time to sleep (in ticks).
//! @return FX_STATUS_OK in normal case, error code otherwise.
//!
int
fx_thread_sleep(uint32_t ticks)
{
    int error = FX_STATUS_OK;
    fx_thread_t* const me = fx_thread_self();
    fx_timer_internal_t* const timer = &me->timer;
    fx_event_internal_t* const timeout_event = &me->timer_event;

    lang_param_assert(
        ticks < FX_TIMER_MAX_RELATIVE_TIMEOUT || 
            ticks == FX_THREAD_INFINITE_TIMEOUT, 
        FX_THREAD_INVALID_TIMEOUT
    );
    lang_param_assert(ticks != 0, FX_STATUS_OK);
    trace_thread_sleep(&me->trace_handle, ticks);
    fx_event_internal_reset(timeout_event);

    if (ticks != FX_THREAD_INFINITE_TIMEOUT)
    {
        fx_timer_internal_set_rel(timer, ticks, 0);
    }

    error = fx_thread_wait_object_internal(
        me, 
        fx_internal_event_as_waitable(timeout_event), 
        NULL, 
        NULL
    );

    //
    // Cancel timer, it may be active in case when thread is resumed by incoming 
    // APC.
    //
    fx_timer_internal_cancel(timer);
    return error;
}

//!
//! Sets thread scheduling parameters.
//! @param [in] thread Thread object to set parameters to.
//! @param [in] type Type of parameter to change (priority, timeslice or CPU).
//! @param [in] value Value of appropriate type (priority, timeslice or CPU).
//! @return FX_STATUS_OK if succeeded, error code otherwise.
//!
int
fx_thread_set_params(fx_thread_t* thread, unsigned int type, unsigned int value)
{
    int error = FX_STATUS_OK;
    fx_sched_state_t prev;
    lang_param_assert(thread != NULL, FX_THREAD_INVALID_PTR);
    lang_param_assert(fx_thread_is_valid(thread), FX_THREAD_INVALID_OBJ);
    
    fx_sched_lock(&prev);

    switch (type)
    {
    case FX_THREAD_PARAM_PRIO:
        if (value < FX_SCHED_ALG_PRIO_IDLE)
        {
            fx_sched_params_t params;
            fx_sched_params_init_prio(&params, value);
            fx_spl_spinlock_get_from_sched(&thread->state_lock);
            fx_sched_item_set_params(&thread->sched_item, &params);
            fx_spl_spinlock_put_from_sched(&thread->state_lock);
            trace_thread_sched_param_set(
                &thread->trace_handle, 
                fx_sched_params_as_number(params)
            );
        }
        else
        {
            error = FX_THREAD_INVALID_PRIO;
        }
        break;

    case FX_THREAD_PARAM_TIMESLICE:
        error = fx_thread_timeslice_set(&thread->timeslice, value) ? 
            FX_STATUS_OK : 
            FX_THREAD_INVALID_TIMESLICE;
        break;

    case FX_THREAD_PARAM_CPU:
        if (value != 0)
        {
            const fx_sched_affinity_t affinity = value;
            fx_thread_lock(thread);
            fx_sched_set_affinity(
                &thread->sched_item, 
                &affinity, 
                fx_thread_self() == thread
            );
            fx_thread_unlock(thread);
        }
        else
        { 
            error = FX_THREAD_INVALID_CPU;
        }
        break;

    default: error = FX_THREAD_INVALID_PARAM; break;
    };

    fx_sched_unlock(prev);
    return error;
}

//!
//! Getting of thread scheduling parameters.
//! @param [in] thread Thread to get parameters from.
//! @param [in] type Type of parameter to get (priority, timeslice or CPU).
//! @param [in] value Pointer to value of appropriate type (priority, 
//! timeslice or CPU number).
//! @return FX_STATUS_OK if succeeded, error code otherwise.
//!
int
fx_thread_get_params(fx_thread_t* thread, unsigned int type,unsigned int* value)
{
    int error = FX_STATUS_OK;
    fx_sched_params_t params;
    fx_sched_state_t prev;  
    fx_sched_affinity_t affinity = 1;

    lang_param_assert(thread != NULL, FX_THREAD_INVALID_PTR);
    lang_param_assert(value != NULL, FX_THREAD_INVALID_PTR);
    lang_param_assert(fx_thread_is_valid(thread), FX_THREAD_INVALID_OBJ);

    fx_sched_lock(&prev);

    switch (type)
    {
    case FX_THREAD_PARAM_PRIO:
        fx_sched_item_get_params(&thread->sched_item, &params);
        *value = fx_sched_params_as_number(&params);
        break;

    case FX_THREAD_PARAM_TIMESLICE:
        *value = thread->timeslice;
        break;

    case FX_THREAD_PARAM_CPU:
        fx_sched_get_affinity(&thread->sched_item, &affinity);
        *value = (unsigned int) affinity;
        break;

    default: error = FX_THREAD_INVALID_PARAM; break;
    };

    fx_sched_unlock(prev);
    return error;
}

//!
//! Used to join (waiting until thread completes).
//! @param [in] thread Thread to join.
//! @param [out] exit_code If non-NULL, thread exit status will be returned here
//! @return FX_STATUS_OK in case of success.
//!
int
fx_thread_join(fx_thread_t* thread)
{
    fx_thread_t* me = fx_thread_self();
    lang_param_assert(thread != NULL, FX_THREAD_INVALID_PTR);
    lang_param_assert(fx_thread_is_valid(thread), FX_THREAD_INVALID_OBJ);
    lang_param_assert(me != thread, FX_THREAD_JOIN_SELF);

    return fx_thread_wait_object_internal(
        me, 
        fx_internal_event_as_waitable(&thread->completion), 
        NULL, 
        NULL
    );
}

//!
//! Stops thread execution until specified timeout reached.
//! @param [in,out] prev_wake Wakeup time, should be initialized by caller 
//! before call.
//! @param [in] increment Time increment.
//! @return FX_STATUS_OK in normal case, FX_THREAD_WAIT_INTERRUPTED in case if 
//! sleeping was interrupted by incoming APC.
//!
int
fx_thread_delay_until(uint32_t* prev_wake, uint32_t increment)
{
    int error = FX_THREAD_INVALID_PARAM;

    fx_thread_t* const me = fx_thread_self();
    fx_timer_internal_t* const timer = &me->timer;
    fx_event_internal_t* const timeout_event = &me->timer_event;
    const uint32_t time_to_wake = *prev_wake + increment;

    lang_param_assert(prev_wake != NULL, FX_THREAD_INVALID_PTR);
    lang_param_assert(
        increment < FX_TIMER_MAX_RELATIVE_TIMEOUT, 
        FX_THREAD_INVALID_TIMEOUT
    );

    *prev_wake = time_to_wake;
    trace_thread_delay_until(&me->trace_handle, time_to_wake);
    fx_event_internal_reset(timeout_event);
    if (!fx_timer_internal_set_abs(timer, time_to_wake, 0))
    {
        error = fx_thread_wait_object_internal(
            me, 
            fx_internal_event_as_waitable(timeout_event), 
            NULL, 
            NULL
        );
        fx_timer_internal_cancel(timer);
    }
    return error;
}

//!
//! Yields execution to another thread with same priority.
//! @warning Yield should be supported by the scheduler.
//!
void
fx_thread_yield(void)
{
    fx_sched_state_t prev;
    fx_sched_lock(&prev);
    fx_sched_yield(fx_thread_as_sched_item(fx_thread_self()));
    fx_sched_unlock(prev);
}

//!
//! Sends external APC message to specified thread.
//! @param [in] thread Thread to send APC to.
//! @param [in] msg APC message to be sent to the thread.
//! @warning If the thread is suspended or waiting it will be made runnable.
//! SPL >= SCHED_LEVEL
//!
bool
fx_thread_send_apc(fx_thread_t* thread, fx_thread_apc_msg_t* msg)
{
    bool accepted = false;
    fx_spl_spinlock_get_from_sched(&thread->state_lock);
    
    //
    // APC may be sent only to threads which is not completed.
    // Thread state is not analyzed since tries to insert APC into completed 
    // thread will be failed.
    //
    if (fx_thread_apc_insert(&thread->apcs, msg, &accepted))
    {
        const unsigned thread_cpu = fx_sched_get_cpu(&thread->sched_item);
        fx_sched_item_resume(&thread->sched_item);
        thread->state = FX_THREAD_STATE_READY;

        //
        // Request rescheduling in order to force APC delivery in case when
        //  target thread was in ready state and resuming took no effect.
        //
        hal_mp_request_ipi(thread_cpu, SPL_DISPATCH);    
    }

    fx_spl_spinlock_put_from_sched(&thread->state_lock);  
    return accepted;
}
