#ifndef _FX_TIMER_INTERNAL_SIMPLE_HEADER_
#define _FX_TIMER_INTERNAL_SIMPLE_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_timer_internal.h
  *  @brief  Internal timers interface. This implementation may be used ONLY in 
  *  uniprocessor systems with unified interrupt architecture since timer 
  *  callback may be called from interrupt handlers directly.
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
#include FX_INTERFACE(RTL_LIST)
#include FX_INTERFACE(HAL_CLOCK)

//
// Error codes.
//
enum
{
    FX_TIMER_OK = FX_STATUS_OK,
    FX_TIMER_ALREADY_CANCELLED,
    FX_TIMER_CONCURRENT_USE,
    FX_TIMER_INTERNAL_ERR_MAX
};

#define FX_TIMER_MAX_RELATIVE_TIMEOUT UINT32_C(0x7FFFFFFF)

//!
//! Timer representation. 
//!
typedef struct
{
    uint32_t timeout;
    uint32_t period;
    int (*callback)(void*);
    void* callback_arg;
    rtl_list_linkage_t link;
} 
fx_timer_internal_t;

void fx_timer_ctor(void);
#define fx_app_timer_ctor()
#define fx_timer_time_after(a, b) (((int32_t)(b) - (int32_t)(a)) < 0)
#define fx_timer_time_after_or_eq(a, b) (((int32_t)(b) - (int32_t)(a)) <= 0)
uint32_t fx_timer_get_tick_count(void);
uint32_t fx_timer_set_tick_count(uint32_t);
int fx_timer_internal_init(fx_timer_internal_t* t, int (*f)(void*), void* arg);
int fx_timer_internal_cancel(fx_timer_internal_t* t);
int fx_timer_internal_set_rel(
    fx_timer_internal_t* t, 
    uint32_t delay, 
    uint32_t period
);
int fx_timer_internal_set_abs(
    fx_timer_internal_t* t, 
    uint32_t delay, 
    uint32_t period
);

FX_METADATA(({ interface: [FX_TIMER_INTERNAL, SIMPLE] }))

#endif
