/** 
  ******************************************************************************
  *  @file   fx_timer.h
  *  @brief  Lightweight implementation of uniprocessor timers.
  *  Timers are in queue sorted by deadline.
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

#include FX_INTERFACE(FX_TIMER_INTERNAL)
#include FX_INTERFACE(TRACE_CORE)
#include FX_INTERFACE(FX_DBG)
#include FX_INTERFACE(FX_SPL)
#include FX_INTERFACE(HAL_MP)

FX_METADATA(({ implementation: [FX_TIMER_INTERNAL, SIMPLE] }))

//
// Simple timers may only be used with unified sync scheme on single-CPU systems
//
lang_static_assert(FX_SPL_SCHED_LEVEL == SPL_SYNC);
lang_static_assert(HAL_MP_CPU_MAX == 1);

static rtl_list_t fx_timer_internal_timers;
static volatile uint32_t fx_timer_internal_ticks = 0;

//!
//! Timer module initialization.
//! @remark SPL = SYNC
//!
void 
fx_timer_ctor(void)
{
    rtl_list_init(&(fx_timer_internal_timers));
}

//!
//! Read tick counter.
//! Since tick counter is 32-bit, reads may not be atomic on 16-bit CPUs, so, 
//! read counter with interrupts disabled.
//! @return current value of tick counter.
//!
uint32_t 
fx_timer_get_tick_count(void)
{
    uint32_t ticks;
    fx_lock_intr_state_t state;
    fx_spl_raise_to_sync_from_any(&state);
    ticks = fx_timer_internal_ticks;
    fx_spl_lower_to_any_from_sync(state);
    return ticks;
}

//!
//! Sets tick counter.
//! Since tick counter is 32-bit, writes may not be atomic on 16-bit CPUs, so, 
//! set counter with interrupts disabled.
//! @return Old value of tick counter.
//!
uint32_t 
fx_timer_set_tick_count(uint32_t newticks)
{
    uint32_t ticks;
    fx_lock_intr_state_t state;
    fx_spl_raise_to_sync_from_any(&state);
    ticks = fx_timer_internal_ticks;
    fx_timer_internal_ticks = newticks;
    fx_spl_lower_to_any_from_sync(state);
    return ticks;
}

//!
//! Timer constructor.
//! Initializes timer object.
//! @param [in,out] timer Timer object to be initialized (allocated by user).
//! @param [in] func Callback function.
//! @param [in] arg Callback argument.
//! @return FX_TIMER_OK if succeeded, error code otherwise.
//!
int 
fx_timer_internal_init(fx_timer_internal_t* timer, int (*fn)(void*), void* arg)
{
    timer->callback = fn;
    timer->callback_arg = arg;
    return FX_TIMER_OK;
}

//!
//! Cancels timer.
//! If timer was inactive, no actions will be performed.
//! @param [in] timer Timer object to be cancelled.
//! @return FX_TIMER_OK in case of success, error code otherwise.
//!
int
fx_timer_internal_cancel(fx_timer_internal_t* timer)
{
    int error = FX_TIMER_ALREADY_CANCELLED;
    fx_lock_intr_state_t state;
    
    fx_spl_raise_to_sync_from_any(&state);
    if (rtl_list_is_node_linked(&timer->link))
    {
        rtl_list_remove(&timer->link);
        error = FX_TIMER_OK;
    }
    fx_spl_lower_to_any_from_sync(state);
    return error;
}

//
// Helper function for timer insertion. It has O(n) latency.
//
static void
_fx_timer_insert(fx_timer_internal_t* timer)
{
    rtl_list_t* head = &(fx_timer_internal_timers);
    rtl_list_t* n = NULL;

    for (n = rtl_list_first(head); n != head; n = rtl_list_next(n)) 
    {
        fx_timer_internal_t* t = rtl_list_entry(n, fx_timer_internal_t, link);
        if (fx_timer_time_after(t->timeout, timer->timeout))
        {
            break;
        }
    }

    rtl_list_insert(rtl_list_prev(n), &timer->link);  
}

//!
//! Sets timeout for timer with specified absolute tick value.
//! @param [in] timer Timer object to be armed.
//! @param [in] delay Absolute timeout value in ticks.
//! @param [in] period Period for periodic timers. 0 for one-shot timers.
//! @return FX_TIMER_OK in case of success, error code otherwise.
//!
int
fx_timer_internal_set_abs(
    fx_timer_internal_t* timer, 
    uint32_t delay, 
    uint32_t period)
{
    fx_lock_intr_state_t state;
    fx_spl_raise_to_sync_from_any(&state);

    if (rtl_list_is_node_linked(&timer->link))
    {
        rtl_list_remove(&timer->link);
    }

    timer->timeout = delay;
    timer->period = period;
    _fx_timer_insert(timer);
    fx_spl_lower_to_any_from_sync(state);
    return FX_TIMER_OK;
}

//!
//! Sets timeout for timer with specified relative tick value.
//! @param [in] timer Timer object to be armed.
//! @param [in] delay Relative timeout value in ticks.
//! @param [in] period Period for periodic timers. 0 for one-shot timers.
//! @return FX_TIMER_OK in case of success, error code otherwise.
//!
int 
fx_timer_internal_set_rel(
    fx_timer_internal_t* timer, 
    uint32_t delay, 
    uint32_t period)
{ 
    uint32_t ticks;
    fx_lock_intr_state_t state;
    fx_spl_raise_to_sync_from_any(&state);
    ticks = fx_timer_internal_ticks;
    fx_spl_lower_to_any_from_sync(state);
    return fx_timer_internal_set_abs(timer, ticks + delay, period);
}

//!
//! Tick handler is called by the HAL.
//!
void 
fx_tick_handler(void)
{
    rtl_list_t* list = &(fx_timer_internal_timers);
    fx_lock_intr_state_t state;

    fx_spl_raise_to_sync_from_any(&state);
    ++fx_timer_internal_ticks;
    trace_increment_tick(fx_timer_internal_ticks);

    while (!rtl_list_empty(list))
    {
        fx_timer_internal_t* item = rtl_list_entry(
            rtl_list_first(list), 
            fx_timer_internal_t, 
            link
        );

        if (!fx_timer_time_after_or_eq(fx_timer_internal_ticks, item->timeout))
        {
            break;
        }

        rtl_list_remove(&item->link);

        if (item->period)
        {
            item->timeout += item->period;
            _fx_timer_insert(item);
        } 

        fx_spl_lower_to_any_from_sync(state);
        (item->callback)(item->callback_arg);
        fx_spl_raise_to_sync_from_any(&state);
    }

    fx_spl_lower_to_any_from_sync(state);
}
