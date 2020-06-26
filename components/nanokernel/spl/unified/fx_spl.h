#ifndef _FX_SPL_UNIFIED_UP_HEADER_
#define _FX_SPL_UNIFIED_UP_HEADER_

/** 
  ******************************************************************************
  *  @file   unified/fx_spl.h
  *  @brief  Definitions for unified synchronization model for uniprocessors.
  *  In this sync scheme OS kernel operates at sync level with intrs disabled.
  *  Spinlocks raise level to sync (disabling interrupts) from any level.
  *  Interrupts handlers run at level higher than DISPATCH and lower than SYNC, 
  *  therefore, OS services may be used from interrupts directly.
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
#include FX_INTERFACE(HAL_ASYNC)
#include FX_INTERFACE(FX_DBG)
#include FX_INTERFACE(TRACE_LOCKS)
#include FX_INTERFACE(HAL_MP)

#define FX_SPL_SCHED_LEVEL SPL_SYNC

typedef struct { spl_t old_spl; } lock_t;
typedef spl_t fx_lock_intr_state_t;

static inline void
fx_spl_raise_to_sync_from_any(spl_t* old_state) 
{
    *old_state = hal_async_raise_spl(SPL_SYNC); 
    trace_intr_lock();
}

static inline void
fx_spl_lower_to_any_from_sync(spl_t old_state) 
{
    trace_intr_unlock();
    hal_async_lower_spl(old_state);
}

//
// In unified architecture scheduler works at level above all software and 
// hardware interrupts, so, "higher" level is not guarantee SYNC. Locks from 
// higher level on uniprocessor system should raise SPL.
//

#define fx_spl_spinlock_init(lock) (lock)->old_spl = SPL_LOW
#define fx_spl_spinlock_get_from_sched(lock) ((void) (lock))
#define fx_spl_spinlock_put_from_sched(lock) ((void) (lock))

static inline void 
fx_spl_spinlock_get_from_any(lock_t* lock)
{
    fx_dbg_assert(hal_async_get_current_spl() != SPL_LOW);
    fx_spl_raise_to_sync_from_any(&lock->old_spl);
}

static inline void 
fx_spl_spinlock_put_from_any(lock_t* lock) 
{
    fx_spl_lower_to_any_from_sync(lock->old_spl);
}

static inline void 
fx_spl_raise_to_sched_from_low(spl_t* prev_state)
{
    fx_spl_raise_to_sync_from_any(prev_state);
}

static inline void 
fx_spl_lower_to_low_from_sched(spl_t prev_state) 
{
    fx_spl_lower_to_any_from_sync(prev_state);
}

static inline void 
fx_spl_raise_to_sched_from_disp(spl_t* prev_state)
{
    fx_spl_raise_to_sync_from_any(prev_state);
}

static inline void
fx_spl_lower_to_disp_from_sched(spl_t prev_state)
{
    fx_spl_lower_to_any_from_sync(prev_state);
}

lang_static_assert(SPL_DISPATCH == SPL_SYNC);
lang_static_assert(HAL_MP_CPU_MAX == 1);

FX_METADATA(({ interface: [FX_SPL, UNIFIED_UP] }))

#endif

