/** 
  ******************************************************************************
  *  @file   fx_timer.h
  *  @brief  User API for timers.
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

#include FX_INTERFACE(FX_TIMER)
#include FX_INTERFACE(HAL_MP)

FX_METADATA(({ implementation: [FX_TIMER, V1] }))

#define FX_TIMER_MAGIC 0x54494D52 // 'TIMR'
#define fx_timer_is_valid(t) (fx_rtp_check((&((t)->rtp)), FX_TIMER_MAGIC))

//!
//! Initialization of timer object.
//! @param timer Timer object to be initialized.
//! @param func Timer callback.
//! @param arg Timer callback argument.
//!
int 
fx_timer_init(fx_timer_t* timer, int (*func)(void*), void* arg)
{
    lang_param_assert(timer != NULL, FX_TIMER_INVALID_PTR);
    lang_param_assert(func != NULL, FX_TIMER_INVALID_CALLBACK);

    fx_rtp_init(&timer->rtp, FX_TIMER_MAGIC);  
    fx_timer_internal_init(&timer->object, func, arg);
    return FX_TIMER_OK;
}

//!
//! Destruction of timer object.
//! @param timer Timer object to be deleted.
//!
int 
fx_timer_deinit(fx_timer_t* timer)
{
    lang_param_assert(timer != NULL, FX_TIMER_INVALID_PTR);
    lang_param_assert(fx_timer_is_valid(timer), FX_TIMER_INVALID_OBJ);

    fx_rtp_deinit(&timer->rtp);
    return FX_TIMER_OK;
}

//!
//! Starts timer with specified period value.
//! @param [in] timer Timer object to be set.
//! @param [in] delay Relative first timeout value in ticks.
//! @param [in] period Period for periodic timers.
//! @return FX_STATUS_OK in case of success, error code otherwise.
//!
int 
fx_timer_set_rel(fx_timer_t* timer, uint32_t delay, uint32_t period)
{
    lang_param_assert(timer != NULL, FX_TIMER_INVALID_PTR);
    lang_param_assert(fx_timer_is_valid(timer), FX_TIMER_INVALID_OBJ);
    lang_param_assert(
        delay < FX_TIMER_MAX_RELATIVE_TIMEOUT, 
        FX_TIMER_INVALID_TIMEOUT
    );
    lang_param_assert(
        period < FX_TIMER_MAX_RELATIVE_TIMEOUT, 
        FX_TIMER_INVALID_TIMEOUT
    );

    return fx_timer_internal_set_rel(&timer->object, delay, period);
}

//!
//! Starts timer with specified absolute time and period value.
//! @param [in] timer Timer object to be set.
//! @param [in] delay Absolute first timeout value in ticks.
//! @param [in] period Period for periodic timers.
//! @return FX_STATUS_OK in case of success, error code otherwise.
//!
int 
fx_timer_set_abs(fx_timer_t* timer, uint32_t delay, uint32_t period)
{
    lang_param_assert(timer != NULL, FX_TIMER_INVALID_PTR);
    lang_param_assert(fx_timer_is_valid(timer), FX_TIMER_INVALID_OBJ);
    lang_param_assert(
        period < FX_TIMER_MAX_RELATIVE_TIMEOUT, 
        FX_TIMER_INVALID_TIMEOUT
    );

    return fx_timer_internal_set_abs(&timer->object, delay, period);
}

//!
//! Cancels timer.
//! If timer was inactive, no actions will be performed.
//! @param [in] timer Timer object to be cancelled.
//! @return FX_TIMER_OK in case if timer has been actually extracted from queue,
//! error code otherwise.
//!
int 
fx_timer_cancel(fx_timer_t* timer)
{
    lang_param_assert(timer != NULL, FX_TIMER_INVALID_PTR);
    lang_param_assert(fx_timer_is_valid(timer), FX_TIMER_INVALID_OBJ);
    
    return fx_timer_internal_cancel(&timer->object);
}
