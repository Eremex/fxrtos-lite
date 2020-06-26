#ifndef _FX_TIMER_V1_HEADER_
#define _FX_TIMER_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_timer.h
  *  @brief  Interface of application timers with enabled error checking.
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

#include FX_INTERFACE(FX_APP_TIMER)
#include FX_INTERFACE(FX_RTP)

enum
{
    FX_TIMER_INVALID_PTR = FX_TIMER_INTERNAL_ERR_MAX,
    FX_TIMER_INVALID_OBJ,
    FX_TIMER_INVALID_TIMEOUT,
    FX_TIMER_INVALID_CALLBACK, 
    FX_TIMER_ERR_MAX 
};

//!
//! Timer representation. 
//!
typedef struct
{
    fx_rtp_t rtp;
    fx_timer_internal_t object;
}
fx_timer_t;

int fx_timer_init(fx_timer_t* timer, int (*func)(void*), void* arg);
int fx_timer_deinit(fx_timer_t* timer);
int fx_timer_set_rel(fx_timer_t* timer, uint32_t delay, uint32_t period);
int fx_timer_set_abs(fx_timer_t* timer, uint32_t delay, uint32_t period);
int fx_timer_cancel(fx_timer_t* timer);

FX_METADATA(({ interface: [FX_TIMER, V1] }))

#endif
