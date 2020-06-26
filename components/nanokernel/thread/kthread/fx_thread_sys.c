/** 
  ******************************************************************************
  *  @file   fx_thread_sys.c
  *  @brief  Implementation of system part of threading subsystem.
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

#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(FX_DBG)
#include FX_INTERFACE(HAL_MP)
#include FX_INTERFACE(FX_DPC)
#include FX_INTERFACE(FX_THREAD_TIMESLICE)

FX_METADATA(({ implementation: [FX_THREAD, V1] }))

//!
//! Context of threading module. Each processor has its own one.
//!
typedef struct
{
    fx_thread_t idle_thread;
    fx_thread_timeslice_context_t timeslicing_context;
} 
fx_thread_context_t;

static fx_thread_context_t g_thread_context[HAL_MP_CPU_MAX];
static fx_thread_t* g_current_thread[HAL_MP_CPU_MAX];

void fx_thread_exit_sync(fx_thread_t* thread);

//!
//! Returns pointer to current thread.
//! @warning In order to prevent thread migration within execution of the 
//! function, it raises SPL level to DISPATCH. So, it cannot be used within 
//! critical sections guarded by spinlocks, in  interrupt handlers, and so on.
//! If it is needed to know current thread in guarded region, call this function
//! before SPL is raised.
//!
fx_thread_t*
fx_thread_self(void)
{
    fx_thread_t* me;

    //
    // SPL should be raised to DISPATCH level before getting of thread module 
    // context in order to prevent thread migration after context read.
    //
#if HAL_MP_CPU_MAX > 1
    fx_sched_state_t prev;
    fx_sched_lock(&prev);
#endif    

    me = g_current_thread[hal_mp_get_current_cpu()];

#if HAL_MP_CPU_MAX > 1
    fx_sched_unlock(prev);
#endif 
    return me;
}

//
// This callback is called by the timer at DISPATCH level on each timeslice 
// expiration.
//
static int 
fx_thread_quanta_expired(void* arg)
{
    fx_sched_state_t prev_state;
    fx_sched_lock_from_disp_spl(&prev_state);
    fx_sched_yield(&(g_current_thread[hal_mp_get_current_cpu()]->sched_item));
    fx_sched_unlock_from_disp_spl(prev_state);
    return 0;
}

//!
//! Thread termination handler. It is called in context of target thread at 
//! SPL = LOW level.
//! @warning Some implementations of APC API for uniprocessor systems handle 
//! APCs at dispatch level in context of calling thread! For these 
//! implementations cleanup handlers must be disabled.
//! @remark APCs are masked here.
//!
void 
fx_thread_term_handler(void* target, unsigned int exception_id, void* arg)
{
    fx_thread_t* const me = lang_containing_record(target, fx_thread_t, apcs);
    fx_sched_state_t prev;

    //
    // Since cleanup handlers may use blocking operations, handle them before 
    // SPL is raised.
    //
    fx_thread_cleanup_handle(&me->cleanup);
    fx_sched_lock(&prev);
    fx_thread_exit_sync(me);
    fx_sched_unlock(prev);
}

//!
//! Used by APC implementation BEFORE calling of APC handler. Should be used to 
//! implement correct "rollback" operation. Thread may be in sleep state when 
//! APC has to be  received, so, this callback should cancel all side-effects 
//! like timers, links to synchronization objects and so on, before APC handler 
//! gets control.
//! SPL = SCHED_LEVEL
//!
static void 
fx_thread_apc_on_receive_handler(fx_thread_apc_target_t* target)
{
    fx_thread_t* const me = lang_containing_record(target, fx_thread_t, apcs);
    fx_timer_internal_cancel(&me->timer);
    fx_sync_wait_rollback(&me->waiter);  
}

//!
//! Thread module constructor.
//! This function must be called on each processor in the system during 
//! initialization.
//! @remark SPL = SYNC
//!
void
fx_thread_ctor(void)
{
    const unsigned int cpu = hal_mp_get_current_cpu();
    fx_thread_context_t* const context = &g_thread_context[cpu];
    fx_thread_t* const idle_thread = &context->idle_thread;
    const fx_sched_affinity_t this_cpu_only = 
        UINT64_C(1) << hal_mp_get_current_cpu();

    g_current_thread[cpu] = idle_thread;

    fx_process_ctor();
    fx_dpc_ctor();
    fx_sched_ctor();
    fx_timer_ctor();

    //
    // Construct idle thread. Because idle thread never sleeps there is no need 
    // to initialize  wait-related stuff like waiter object, timer and so on. 
    // Only objects used by dispatching  must be initialized.
    // These objects include: RTP, scheduler item, context, stack control, 
    // APC and timeslice.
    //
    fx_rtp_init(&idle_thread->rtp, FX_THREAD_MAGIC);
    fx_sched_item_init(&idle_thread->sched_item,FX_SCHED_PARAMS_INIT_IDLE,NULL);
    fx_sched_item_add(&idle_thread->sched_item);
    fx_sched_set_affinity(&idle_thread->sched_item, &this_cpu_only, true);
    fx_sched_item_resume(&idle_thread->sched_item);
    fx_stackovf_init(&idle_thread->stk_info, NULL, 0); 
    fx_thread_apc_target_init(&idle_thread->apcs);
    fx_thread_timeslice_ctor(
        &context->timeslicing_context, 
        fx_thread_quanta_expired, 
        NULL
    );
    trace_thread_init_idle(&idle_thread->trace_handle, FX_SCHED_ALG_PRIO_IDLE);
    idle_thread->parent = fx_process_self();

    if (hal_mp_get_current_cpu() == 0)
    {
        fx_process_set_exception(FX_EXCEPTION_TERM,fx_thread_term_handler,NULL);
        fx_thread_apc_ctor(&idle_thread->apcs,fx_thread_apc_on_receive_handler);
    }

    fx_app_timer_ctor();
}

//!
//! Thread dispatch procedure. It is DISPATCH software interrupt handler.
//!
void 
fx_dispatch_handler(void)
{
    fx_sched_item_t* item = NULL;
    fx_sched_state_t prev_state;
    const unsigned int cpu = hal_mp_get_current_cpu();

    fx_dpc_handle_queue();

    //
    // For unified interrupt scheme this call raise SPL to SCHED_LEVEL and 
    // disables interrupts, For segmented one this call does nothing, since 
    // we're already at DISPATCH level which is SCHED_LEVEL.
    //
    fx_sched_lock_from_disp_spl(&prev_state);

    //
    // Schedule next thread. If NEXT is equal to NULL it means no rescheduling 
    // was requested and current thread stays the same.
    //
    item = fx_sched_get_next();

    if (item)
    {
        fx_thread_t* const next = lang_containing_record(
            item, 
            fx_thread_t, 
            sched_item
        );
        fx_thread_context_t* const context = &g_thread_context[cpu];
        fx_thread_t* const prev = g_current_thread[cpu];

        fx_thread_timeslice_reset(
            &context->timeslicing_context, 
            &next->timeslice, 
            &prev->timeslice,
            !fx_sched_params_is_unique(fx_sched_item_as_sched_params(item))
        );

        if (next != prev)
        {
            g_current_thread[cpu] = next;
            
            trace_thread_context_switch(
                &prev->trace_handle, 
                &next->trace_handle
            );
            
            fx_process_switch(next->parent, prev->parent);
            hal_context_switch(&next->hw_context, &prev->hw_context);
            fx_stackovf_check(&next->stk_info, hal_intr_frame_get());
            
            //
            // Completion event must be set from the scheduler in order to avoid
            // race conditions on kernel threads termination. It should be 
            // guaranteed that the thread will no longer be used by anyone, when
            // the event is set. This is the reason why thread cannot set it by 
            // itself.
            //
            if (prev->state == FX_THREAD_STATE_COMPLETED)
            {
                fx_event_internal_set(&prev->completion);
                fx_thread_cleanup_switch_hook(&prev->cleanup);
            }
        }
    }
 
    fx_thread_apc_deliver(&(g_current_thread[cpu]->apcs));    
    fx_sched_unlock_from_disp_spl(prev_state);
}  

//!
//! This function is called by HAL in context of thread causing the exception.
//!
void 
fx_trap_handler(unsigned int exc_id, void* arg) 
{
    fx_thread_t* const me = fx_thread_self();
    fx_process_exception_handler_t handler = fx_process_get_exception(exc_id);
    void* handler_arg = arg;
    unsigned int id = exc_id;

    //
    // If handler is not specified, then TERM handler is called (which must be 
    // always installed).
    // 
    if (handler == NULL)
    {
        handler = fx_process_get_exception(FX_EXCEPTION_TERM);
        handler_arg = NULL;
        id = FX_EXCEPTION_TERM;
    }

    handler(&me->apcs, id, handler_arg);
}
