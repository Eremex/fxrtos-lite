#ifndef _FX_SYS_TIMER_PROXY_HEADER_
#define _FX_SYS_TIMER_PROXY_HEADER_

/**
  ******************************************************************************
  *  @file   smp/fx_sys_timer.h
  *  @brief  Proxy header for non-threaded timers.
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

//
// For ISR and DPC-based timers systimers are directly mapped to internal timers
//
typedef fx_timer_internal_t fx_sys_timer_t;

#define fx_sys_timer_ctor()
#define fx_sys_timers_tick(ticks)
#define fx_sys_timer_init(t, func, arg) fx_timer_internal_init(t, func, arg)
#define fx_sys_timer_set_relative(t, delay) fx_timer_internal_set_rel(t,delay,0)
#define fx_sys_timer_set_absolute(t, delay) fx_timer_internal_set_abs(t,delay,0)
#define fx_sys_timer_cancel(t) fx_timer_internal_cancel(t)

FX_METADATA(({ interface: [FX_SYS_TIMER, PROXY] }))

#endif
