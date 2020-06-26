#ifndef _FX_SCHED_UP_FIFO_HEADER_
#define _FX_SCHED_UP_FIFO_HEADER_

/**
  ******************************************************************************
  *  @file   fx_sched.h
  *  @brief  Interface header for global uniprocessor scheduler.
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

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(FX_SCHED_ALG)
#include FX_INTERFACE(HAL_ASYNC)
#include FX_INTERFACE(FX_SPL)

//!
//! Schedulable entity.
//!
typedef struct
{
    unsigned int suspend_count;     //!< Item suspended if this value is > 0.
    fx_sched_params_t sched_params; //!< Associated parameters, priority, etc.
}
fx_sched_item_t;

#define fx_sched_item_as_sched_params(item) (&((item)->sched_params))

void fx_sched_ctor(void);

void fx_sched_item_init(
    fx_sched_item_t* item, 
    fx_sched_params_init_t t, 
    const fx_sched_params_t* arg
);

void fx_sched_item_remove(fx_sched_item_t* item);
void fx_sched_item_get_params(fx_sched_item_t* src, fx_sched_params_t* dst);
void fx_sched_item_set_params(fx_sched_item_t* dst, const fx_sched_params_t* s);
unsigned int fx_sched_item_suspend(fx_sched_item_t* item);
unsigned int fx_sched_item_resume(fx_sched_item_t* item);
bool fx_sched_yield(fx_sched_item_t* item);
fx_sched_item_t* fx_sched_get_next(void);
void fx_sched_mark_resched_needed(void);

//
// Add schedulable item to the scheduler. 
// In current implementation actual work is done in @ref fx_sched_item_resume.
//
#define fx_sched_item_add(item)

//
// SMP API. On uniprocessor systems it is implemented as stubs.
//
typedef int fx_sched_affinity_t;
#define fx_sched_set_affinity(item, affinity, self) ((void)(*(affinity)))
#define fx_sched_get_affinity(item, affinity) ((void)(*(affinity)))
#define fx_sched_get_cpu(item) 0

typedef spl_t fx_sched_state_t;
#define fx_sched_lock(prev_ptr) fx_spl_raise_to_sched_from_low(prev_ptr)
#define fx_sched_unlock(prev) fx_spl_lower_to_low_from_sched(prev)
#define fx_sched_lock_from_disp_spl(prev) fx_spl_raise_to_sched_from_disp(prev)
#define fx_sched_unlock_from_disp_spl(prv) fx_spl_lower_to_disp_from_sched(prv)

FX_METADATA(({ interface: [FX_SCHED, UP_FIFO] }))

#endif
