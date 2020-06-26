#ifndef _FX_THREAD_V1_HEADER_
#define _FX_THREAD_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_thread.h
  *  @brief  Threads implementation.
  *  The module implements basic threads on top of scheduler's container.
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

#include FX_INTERFACE(HAL_CPU_CONTEXT)
#include FX_INTERFACE(FX_PROCESS)
#include FX_INTERFACE(FX_SCHED)
#include FX_INTERFACE(FX_EVENT)
#include FX_INTERFACE(FX_TIMER_INTERNAL)
#include FX_INTERFACE(FX_THREAD_APC)
#include FX_INTERFACE(FX_THREAD_CLEANUP)
#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(FX_STACKOVF)  
#include FX_INTERFACE(TRACE_CORE) 
  
//!
//! Thread states.
//!
typedef enum
{
    FX_THREAD_STATE_READY = 0,
    FX_THREAD_STATE_SUSPENDED = 1,
    FX_THREAD_STATE_WAITING = 2,
    FX_THREAD_STATE_COMPLETED = 3,
}
fx_thread_state_t;

enum
{
    FX_THREAD_MAGIC = 0x54485244,   // 'THRD'

    //
    // Parameter select for fx_thread_set/get_params
    //
    FX_THREAD_PARAM_PRIO = 0,
    FX_THREAD_PARAM_TIMESLICE = 1,
    FX_THREAD_PARAM_CPU = 2,
    FX_THREAD_PARAM_MAX,

    //
    // API functions status codes.
    //
    FX_THREAD_OK = FX_STATUS_OK,    // Predefined success status.
    FX_THREAD_WAIT_CANCELLED,       // Wait cancelled (due to cancel condition).
    FX_THREAD_WAIT_DELETED,         // Wait aborted due to waitable destruction.
    FX_THREAD_WAIT_INTERRUPTED,     // Wait cancelled due to incoming APC.  
    FX_THREAD_WAIT_TIMEOUT,         // Wait is timed out. 
    FX_THREAD_WAIT_IN_PROGRESS,     // Reserved for internal use.
    FX_THREAD_INVALID_PTR,          // Invalid object pointer.
    FX_THREAD_INVALID_ENTRY,        // Invalid entry function.
    FX_THREAD_INVALID_PRIO,         // Invalid priority value.
    FX_THREAD_INVALID_CPU,          // Invalid CPU selection.
    FX_THREAD_NO_STACK,             // Incorrect stack parameters.
    FX_THREAD_INVALID_OBJ,          // Incorrect stack parameters.
    FX_THREAD_INVALID_TIMEOUT,      // Incorrect timeout value.
    FX_THREAD_INVALID_PARAM,        // Incorrect parameter.
    FX_THREAD_JOIN_SELF,            // Incorrect parameter.
    FX_THREAD_INVALID_TIMESLICE,    // Incorrect parameter.
    FX_THREAD_ERR_MAX,      
};

//!
//! Thread representation.
//!
typedef struct
{
    fx_rtp_t rtp;
    fx_process_t* parent;
    fx_sync_waiter_t waiter;
    fx_sched_item_t sched_item;
    uint32_t timeslice;
    fx_thread_apc_target_t apcs;
    fx_thread_cleanup_context_t cleanup;
    fx_timer_internal_t timer;
    fx_event_internal_t timer_event;
    fx_event_internal_t completion;
    hal_cpu_context_t hw_context;
    fx_stackovf_info_t stk_info;
    lock_t state_lock;
    fx_thread_state_t state;
    bool is_terminating;
    trace_thread_handle_t trace_handle;
}
fx_thread_t;

//
// Internal API functions (not intended to be used by applications).
//
#define fx_thread_as_cleanup_context(t) (&((t)->cleanup))
#define fx_thread_lock(t) fx_spl_spinlock_get_from_sched(&((t)->state_lock))
#define fx_thread_unlock(t) fx_spl_spinlock_put_from_sched(&((t)->state_lock))
#define fx_thread_as_sched_item(thread) (&((thread)->sched_item))
#define fx_thread_as_sched_params(thread) \
    (fx_sched_item_as_sched_params(fx_thread_as_sched_item(thread)))
void fx_thread_ctor(void);
int fx_thread_wait_object(fx_sync_waitable_t* w, void* attr, fx_event_t* ev);
int fx_thread_timedwait_object(fx_sync_waitable_t* w, void* attr, uint32_t tm);
bool fx_thread_send_apc(fx_thread_t* thread, fx_thread_apc_msg_t* msg);
#define fx_thread_cancel_apc(t, a) fx_thread_apc_cancel(&((t)->apcs), a)
#define fx_thread_enter_critical_region() ((void) fx_thread_apc_set_mask(true))
#define fx_thread_leave_critical_region() ((void) fx_thread_apc_set_mask(false))

//
// Public API.
//
#define FX_THREAD_INFINITE_TIMEOUT UINT32_C(0xFFFFFFFF)
#define fx_thread_init(a, b, c, d, e, f, g) \
    fx_thread_init_ex(fx_process_self(), a, b, c, d, e, f, g)

int fx_thread_init_ex(
    fx_process_t* parent,
    fx_thread_t* thread,
    void (*func)(void*), 
    void* arg, 
    unsigned int priority, 
    void* stack,
    size_t stack_sz, 
    bool create_suspended
);
int fx_thread_deinit(fx_thread_t* thread);
int fx_thread_terminate(fx_thread_t* thread);
void fx_thread_exit(void);
int fx_thread_join(fx_thread_t* thread);
int fx_thread_suspend(void);
int fx_thread_resume(fx_thread_t* thread);
int fx_thread_sleep(uint32_t ticks);
int fx_thread_delay_until(uint32_t* prev_wake, uint32_t increment);
fx_thread_t* fx_thread_self(void);
void fx_thread_yield(void);
int fx_thread_get_params(fx_thread_t* thread, unsigned int t, unsigned int* v);
int fx_thread_set_params(fx_thread_t* thread, unsigned int t, unsigned int v);
int fx_thread_wait_event(fx_event_t* event, fx_event_t* cancel_event);
int fx_thread_timedwait_event(fx_event_t* event, uint32_t timeout);

FX_METADATA(({ interface: [FX_THREAD, V1] }))

#endif
