/** 
  ******************************************************************************
  *  @file   fx_cond.c
  *  @brief  Implementation of condition variables.
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

#include FX_INTERFACE(FX_COND)

FX_METADATA(({ implementation: [FX_COND, V1] }))

#define fx_cond_is_valid(cond) (fx_rtp_check((&((cond)->rtp)), FX_COND_MAGIC))

//!
//! Test and wait function. It is used for wait implementation.
//! @param [in] object Condvar object to be tested.
//! @param [in] wb Wait block to be inserted into queue.
//! @param [in] wait Wait option used to test object. 
//! @return true in case of object is signaled, false otherwise. If object is 
//! nonsignaled it means that wait block has been inserted into the queue.
//! @remark SPL = SCHED_LEVEL
//!
static bool 
fx_cond_test_and_wait(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait)
{
    fx_mutex_t* mutex = fx_sync_wait_block_get_attr(wb);

    // 
    // Condvar is always causes thread to wait, until explicitly signaled by 
    // signal or broadcast.
    //
    if (wait)
    {
        fx_sync_waitable_lock(object);
        _fx_sync_wait_start(object, wb);
        fx_sync_waitable_unlock(object);
    }

    fx_mutex_release(mutex);
    return false;
}

//!
//! Initializes a condvar.
//! @param [in,out] cond Condvar object to be initialized.
//! @param [in] policy Default policy of waiters releasing.
//! @return FX_COND_OK in case of successful init, error code otherwise.
//! @sa fx_cond_deinit
//!
int 
fx_cond_init(fx_cond_t* cond, const fx_sync_policy_t policy)
{
    lang_param_assert(cond != NULL, FX_COND_INVALID_PTR);
    lang_param_assert(policy < FX_SYNC_POLICY_MAX, FX_COND_UNSUPPORTED_POLICY);

    fx_rtp_init(&cond->rtp, FX_COND_MAGIC);
    fx_spl_spinlock_init(&cond->lock);
    cond->policy = policy;
    fx_sync_waitable_init(&cond->waitable, &cond->lock, fx_cond_test_and_wait);
    return FX_COND_OK;
}

//!
//! Destructor of a condvar.
//! @param [in,out] cond Condvar object to be deinitialized.
//! @return FX_COND_OK in case of successful destruction, error code otherwise.
//! @warning This destructor only releases all waiters with appropriate status. 
//! It is not responsible for preventing of using condvar after destruction.
//! @sa fx_cond_init
//!
int 
fx_cond_deinit(fx_cond_t* cond)
{
    fx_sched_state_t prev;
    lang_param_assert(cond != NULL, FX_COND_INVALID_PTR);
    lang_param_assert(fx_cond_is_valid(cond), FX_COND_INVALID_OBJ);

    fx_sched_lock(&prev);
    fx_rtp_deinit(&cond->rtp);
    fx_sync_waitable_lock(&cond->waitable);
    _fx_sync_wait_notify(&cond->waitable, FX_WAIT_DELETED, NULL);
    fx_sync_waitable_unlock(&cond->waitable);
    fx_sched_unlock(prev);
    return FX_COND_OK;
}

//!
//! Signaling of a condvar.
//! It releases one waiting thread in accordance with queue policy 
//! (FIFO or priority-based).
//! @param [in,out] cond Condvar object to be signaled.
//! @param [in] policy Notification policy (FIFO or priority-based).
//! @return FX_COND_OK in case of success, error code otherwise.
//! @sa fx_cond_broadcast
//!
int 
fx_cond_signal_with_policy(fx_cond_t* cond, const fx_sync_policy_t policy)
{
    fx_sched_state_t prev;
    int error = FX_COND_NO_WAITERS;
    lang_param_assert(cond != NULL, FX_COND_INVALID_PTR);
    lang_param_assert(fx_cond_is_valid(cond), FX_COND_INVALID_OBJ);
    lang_param_assert(policy < FX_SYNC_POLICY_MAX, FX_COND_UNSUPPORTED_POLICY);
        
    fx_sched_lock(&prev);    
    fx_sync_waitable_lock(&cond->waitable);
    if (_fx_sync_waitable_nonempty(&cond->waitable))
    {
        fx_sync_wait_block_t* wb = _fx_sync_wait_block_get(
            &cond->waitable, 
            policy
        );
        _fx_sync_wait_notify(&cond->waitable, FX_WAIT_SATISFIED, wb);
        error = FX_COND_OK;
    }
    fx_sync_waitable_unlock(&cond->waitable);
    fx_sched_unlock(prev);
    return error;
}

//!
//! Signaling of a condvar.
//! It releases one waiting thread with default releasing policy.
//! @param [in,out] cond Condvar object to be signaled.
//! @return FX_COND_OK in case of success, error code otherwise.
//! @sa fx_cond_broadcast
//!
int 
fx_cond_signal(fx_cond_t* cond)
{
    int error = fx_cond_signal_with_policy(cond, cond->policy);
    if (error == FX_COND_NO_WAITERS)
    {
        error = FX_COND_OK;
    }
    return error;
}

//!
//! Broadcast signaling of a condvar.
//! Releasing all threads, who starts the wait before this function has called.
//! Threads who blocks after (including this moment) will not be released.
//! @param [in,out] cond Condvar object to be signaled.
//! @return FX_COND_OK in case of success, error code otherwise.
//! @sa fx_cond_signal
//!
int 
fx_cond_broadcast(fx_cond_t* cond)
{
    fx_sched_state_t prev;
    lang_param_assert(cond != NULL, FX_COND_INVALID_PTR);
    lang_param_assert(fx_cond_is_valid(cond), FX_COND_INVALID_OBJ);

    fx_sched_lock(&prev);
    fx_sync_waitable_lock(&cond->waitable);
    _fx_sync_wait_notify(&cond->waitable, FX_WAIT_SATISFIED, NULL);
    fx_sync_waitable_unlock(&cond->waitable);
    fx_sched_unlock(prev);
    return FX_COND_OK;
}

//!
//! Wait for condvar signal.
//! It blocks calling thread until either condvar became signaled or cancel 
//! event is set.
//! @param [in,out] cond Condvar object to wait for.
//! @param [in,out] mutex Mutex associated with the condvar.
//! @param [in,out] cancel_event Optional cancel event (can be NULL).
//! @return Wait status.
//! @warning Mutex should be acquired by thread calling this function.
//! @sa fx_cond_signal
//! @sa fx_cond_broadcast
//!
int  
fx_cond_wait(fx_cond_t* cond, fx_mutex_t* mutex, fx_event_t* cancel_event)
{
    lang_param_assert(cond != NULL, FX_COND_INVALID_PTR);
    lang_param_assert(fx_cond_is_valid(cond), FX_COND_INVALID_OBJ);  
    lang_param_assert(mutex != NULL, FX_COND_INVALID_MUTEX);
    lang_param_assert(
        fx_mutex_get_owner(mutex) == fx_thread_self(), 
        FX_COND_INVALID_MUTEX
    );
    {
        //
        // Wait for condition variable. It will cause "test_and_wait" virtual 
        // method to be called, which releases the mutex.
        //
        const int cond_wait_res = fx_thread_wait_object(
            &cond->waitable, 
            mutex, 
            cancel_event
        );

        //
        // Before return from cond_wait function the mutex must be re-acquired.
        //
        const int mutex_wait_res = fx_mutex_acquire(mutex, NULL);

        return (mutex_wait_res == FX_MUTEX_OK) ? 
            cond_wait_res : 
            FX_COND_MUTEX_ERROR;
    }
}

//!
//! Wait for condvar signal with timeout.
//! It blocks calling thread until either condvar became signaled or timeout is
//! exceeded.
//! @param [in,out] cond Condvar object to wait for.
//! @param [in,out] mutex Mutex associated with the condvar.
//! @param [in,out] tout Timeout value (in ticks) or FX_THREAD_INFINITE_TIMEOUT.
//! @return Wait status.
//! @warning Mutex should be acquired by thread calling this function.
//! @sa fx_cond_signal
//! @sa fx_cond_broadcast
//!
int  
fx_cond_timedwait(fx_cond_t* cond, fx_mutex_t* mutex, uint32_t tout)
{
    lang_param_assert(cond != NULL, FX_COND_INVALID_PTR);
    lang_param_assert(fx_cond_is_valid(cond), FX_COND_INVALID_OBJ);  
    lang_param_assert(mutex != NULL, FX_COND_INVALID_MUTEX);
    lang_param_assert(
        fx_mutex_get_owner(mutex) == fx_thread_self(), 
        FX_COND_INVALID_MUTEX
    );

    {
        //
        // Wait for condition variable. It will cause "test_and_wait" virtual 
        // method to be called, which releases the mutex.
        //
        const int cond_wait_res = fx_thread_timedwait_object(
            &cond->waitable, 
            mutex, 
            tout
        );
    
        //
        // Mutex will be re-acquired regardless of timeout.
        //
        const int mutex_wait_res = fx_mutex_acquire(mutex, NULL);

        return (mutex_wait_res == FX_STATUS_OK) ? 
            cond_wait_res : 
            FX_COND_MUTEX_ERROR;;
    }
}
