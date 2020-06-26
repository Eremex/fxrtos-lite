#ifndef _FX_THREAD_TIMESLICE_ENABLED_HEADER_
#define _FX_THREAD_TIMESLICE_ENABLED_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_thread_timeslice.h
  *  @brief  Interface header for round-robin aspect of thread module.
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

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(FX_SYS_TIMER)
#include FX_INTERFACE(FX_TIMER_INTERNAL)

FX_METADATA(({ interface: [FX_THREAD_TIMESLICE, ENABLED] }))

//!
//! Global (per-processor) context of round-robin scheduling aspect.
//! 
typedef struct
{
    fx_sys_timer_t timer;   //!< System timer for active thread quanta.
    bool timer_armed;       //!< Timer is armed if true.
    int (*yield)(void*);    //!< Callback is used as yield func for the thread.
}
fx_thread_timeslice_context_t;

static inline void 
fx_thread_timeslice_ctor(
    fx_thread_timeslice_context_t* context, 
    int (*f)(void*), 
    void* arg)
{
    fx_sys_timer_init(&context->timer, f, arg);
    context->yield = f;
}

static inline bool
fx_thread_timeslice_set(uint32_t* timeslice, uint32_t q) 
{
    bool success = false;

    if (q < FX_TIMER_MAX_RELATIVE_TIMEOUT)
    {
        *timeslice = q;
        success = true;
    }

    return success;
}

//!
//! Reset timeslice. It is called by the scheduler every time when new thread is
//! scheduled.
//! @param [in] context Module context. Every processor has its own context.
//! @param [in] next Next timeslice pointer.
//! @param [in] prev previous timeslice pointer.
//! @param [in] not_unique True if there are pending threads with same priority.
//!
static inline void 
fx_thread_timeslice_reset(
    fx_thread_timeslice_context_t* context, 
    uint32_t* next, 
    uint32_t* prev, 
    bool not_unique)
{
    const bool changed = next != prev;

    //
    // Timer should be set in case when next scheduled item has nonzero
    // timeslice and there are pending ready threads at same priority level.
    //
    const bool should_set_timer = *next && not_unique;

    //
    // Cancel the timer if it is active, but should not be active.
    // There are two cases:
    // a) If timer should not be set since scheduled item is unique or has no 
    //    timeslice.
    // b) If scheduled item is changed, since new item may have different 
    //    timeslice.
    //
    if ((changed || !should_set_timer) && context->timer_armed) 
    {
        context->timer_armed = false;
        fx_sys_timer_cancel(&context->timer);

    //
    // There's special case when threaded tickless timers are used.
    // Usually system timers are handled at SPL=DISPATCH and have separate
    // infrastructure in case when timers are handled by thread. This is
    // because quanta timers may not be handled in thread since they always
    // yield timer thread who executes their callbacks instead of thread who
    // sets the quanta. In tickless case it is extremely difficult to support
    // two infrastructures for both application and system timers. So we use 
    // the following trick for thread quantas: System timers are mapped to
    // application timers, timer thread is activated every time when quanta 
    // expires in any other thread and preemption occurs. During context switch
    // this function is called for thread with expired quanta, if we see that
    // cancelled system timer is expired, thread yield function should be called
    // manually. So, timer thread activation is used as quanta trigger.
    // This design introduces extra overhead for every quanta expiration and
    // is considered as a temporary solution.
    //
#if defined FX_APP_TIMER_TICKLESS
        if (fx_timer_time_after_or_eq(
                context->timer.timeout,
                fx_timer_get_tick_count()
            )
        )
        {
            (void) context->yield(prev);
        }
#endif
    }

    if (should_set_timer && !context->timer_armed) 
    {
        context->timer_armed = true;
        fx_sys_timer_set_absolute(
            &context->timer, 
            fx_timer_get_tick_count() + *next
        );
    }
}

#endif
