/** 
  ******************************************************************************
  *  @file   fx_barrier.c
  *  @brief  Implementation of barriers.
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

#include FX_INTERFACE(FX_BARRIER)

FX_METADATA(({ implementation: [FX_BARRIER, V1] }))

#define fx_barrier_is_valid(barr) \
    (fx_rtp_check(&((barr)->rtp), FX_BARRIER_MAGIC))

//!
//! Virtual function which is called from wait framework to test whether the 
//! barrier is locked or not.
//! @remark SPL = SCHED_LEVEL
//!
static bool 
fx_barrier_test_and_wait(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait)
{
    fx_barrier_t* barr = lang_containing_record(object, fx_barrier_t, waitable);
    bool wait_satisfied = false;

    fx_sync_waitable_lock(object);

    if (++barr->blocked == barr->barrier)
    { 
        void* const attr = fx_sync_wait_block_get_attr(wb);

        barr->blocked = 0;

        //
        // Thread that make barrier unlocked should get special key value.
        //
        *(fx_barrier_key_t*)attr = FX_BARRIER_SERIAL_THREAD;
        _fx_sync_wait_notify(&barr->waitable, FX_WAIT_SATISFIED, NULL);
        wait_satisfied = true;
    }
    else if (wait)
    {
        _fx_sync_wait_start(object, wb);
    }
    
    fx_sync_waitable_unlock(object); 
    return wait_satisfied; 
}

//!
//! Barrier initialization.
//! @param barr Barrier object to be initialized.
//! @param barrier Number of waiting threads to unlock the barrier.
//! @return FX_BARR_OK in case of success, error code otherwise.
//!
int 
fx_barrier_init(fx_barrier_t* barr, const unsigned int barrier)
{
    lang_param_assert(barr != NULL, FX_BARR_INVALID_PTR);
    lang_param_assert(barrier != 0, FX_BARR_ZERO_LIMIT);

    fx_rtp_init(&barr->rtp, FX_BARRIER_MAGIC);
    fx_spl_spinlock_init(&barr->lock);
    fx_sync_waitable_init(
        &barr->waitable, 
        &barr->lock, 
        fx_barrier_test_and_wait
    );

    barr->barrier = barrier;
    barr->blocked = 0;
    return FX_BARR_OK;
}

//!
//! Barrier deinitialization.
//! @param barr Barrier object to be deinitialized.
//! @return FX_BARR_OK in case of success, error code otherwise.
//!
int 
fx_barrier_deinit(fx_barrier_t* barr)
{
    fx_sched_state_t prev;
    lang_param_assert(barr != NULL, FX_BARR_INVALID_PTR);
    lang_param_assert(fx_barrier_is_valid(barr), FX_BARR_INVALID_OBJ);  

    fx_sched_lock(&prev);
    fx_rtp_deinit(&barr->rtp);
    fx_sync_waitable_lock(&barr->waitable);
    _fx_sync_wait_notify(&barr->waitable, FX_WAIT_DELETED, NULL);
    fx_sync_waitable_unlock(&barr->waitable);
    fx_sched_unlock(prev);
    return FX_BARR_OK;
}

//!
//! Waiting for barrier.
//! If calling thread is n-th waiting thread, where n is barrier threashold set 
//! on barrier init, then all waiting threads (including calling thread) will be
//! resumed.
//! @param barr Barrier object.
//! @param key If calling thread is thread who unlocks the barrier, special 
//! value will be placed in key variable, for all oher threads the value will be
//! unchanged (optional).
//! @param cancel Optional cancel event.
//! @return FX_BARR_OK in case of success, error code otherwise.
//! @remark SPL = LOW
//!
int 
fx_barrier_wait(fx_barrier_t* barr, fx_barrier_key_t* key, fx_event_t* cancel)
{
    fx_barrier_key_t k;
    int res = FX_BARR_OK;
    lang_param_assert(barr != NULL, FX_BARR_INVALID_PTR);
    lang_param_assert(fx_barrier_is_valid(barr), FX_BARR_INVALID_OBJ); 

    res = fx_thread_wait_object(&barr->waitable, &k, cancel);

    if (key && res == FX_THREAD_OK && k == FX_BARRIER_SERIAL_THREAD)
    {
        *key = k;
    }
    return res;
}

//!
//! Waiting for barrier with timeout.
//! If calling thread is n-th waiting thread, where n is barrier threashold set 
//! on barrier init, then all waiting threads (including calling thread) will be
//! resumed.
//! @param barr Barrier object.
//! @param key If calling thread is thread who unlocks the barrier, special 
//! value will be placed at key variable, for all oher threads the value will be
//! unchanged (optional).
//! @param tout Timeout value or FX_THREAD_INFINITE_TIMEOUT.
//! @return FX_BARR_OK in case of success, error code otherwise.
//! @remark SPL = LOW
//!
int 
fx_barrier_timedwait(fx_barrier_t* barr, fx_barrier_key_t* key, uint32_t tout)
{
    fx_barrier_key_t k;
    int res = FX_BARR_OK;
    lang_param_assert(barr != NULL, FX_BARR_INVALID_PTR);
    lang_param_assert(fx_barrier_is_valid(barr), FX_BARR_INVALID_OBJ); 

    res = fx_thread_timedwait_object(&barr->waitable, &k, tout);
    if (res == FX_THREAD_OK && key)
    {
        *key = k;
    }
    return res;  
}
