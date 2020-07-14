/** 
  ******************************************************************************
  *  @file   mutex.c
  *  @brief  Implementation of mutexes.
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

#include FX_INTERFACE(FX_MUTEX)
#include FX_INTERFACE(FX_DBG)

FX_METADATA(({ implementation: [FX_MUTEX, V1] }))

#define fx_mutex_is_valid(m) (fx_rtp_check((&((m)->rtp)), FX_MUTEX_MAGIC))

//!
//! Test and wait function.
//! If mutex is free this function also performs priority adjust if it is 
//! enabled for this mutex object.
//! @param [in] object Mutex object to be tested.
//! @param [in] wb Wait block to be inserted into waiters queue if mutex is busy
//! @param [in] wait Wait option used to test object, if it is nonzero wait will
//! actually start in case when the mutex is busy, if it is zero - this 
//! function just tests the object and return status. 
//! @return true in case of object was signaled, false otherwise.
//! @remark SPL = SCHED_LEVEL
//!
static bool 
fx_mutex_test_and_wait(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait)
{
    fx_mutex_t* mutex = lang_containing_record(object, fx_mutex_t, waitable);
    fx_thread_t* me = lang_containing_record(wb->waiter, fx_thread_t, waiter);
    bool acquired = true;
    bool apply_ceiling = false;

    fx_sync_waitable_lock(object);

    do
    {
        if (!mutex->owner)
        {
            mutex->owner = me;
            if (mutex->ceiling_enabled)
            {
                apply_ceiling = true;
            }
            trace_mutex_acquired(&mutex->trace_handle, me->trace_handle);
            break;
        }

        if (mutex->owner == me)
        {
            ++mutex->recursive_locks;
            break;
        }

        if (wait)
        {            
            _fx_sync_wait_start(object, wb); 
            trace_mutex_acquire_block(&mutex->trace_handle);
        }
        acquired = false;
    }
    while (0);

    if (apply_ceiling == true)
    {      
        fx_sched_params_copy(
            fx_thread_as_sched_params(me), 
            &mutex->owner_params
        );

        if (fx_sched_params_is_preempt(
                &mutex->ceiling_orig, 
                fx_thread_as_sched_params(me)))
        {
            fx_thread_lock(me);
            fx_sched_item_set_params(
                fx_thread_as_sched_item(me), 
                &mutex->ceiling_orig
            );
            fx_thread_unlock(me);

            trace_thread_ceiling(
                fx_thread_as_trace_handle(me), 
                fx_sched_params_as_number(&mutex->owner_params),
                fx_sched_params_as_number(&mutex->ceiling_orig)
            );
        }
    }

    fx_sync_waitable_unlock(object);
    return acquired;
}

//!
//! Mutex initialization.
//! @param [in,out] mutex Mutex object to be initialized.
//! @param [in] priority Scheduling priority of the mutex for ceiling. 
//! The priority will be applied to thread that owns this mutex (while mutex 
//! is acquired). If this parameter is FX_MUTEX_CEILING_DISABLED then ceiling is
//! disabled for the mutex.
//! @param [in] policy Default policy of waiter releasing for this mutex.
//! @warning If the ceiling is enabled, be sure that ceiliing priority is equal 
//! (or greater than) to priority of the most prioritized thread, that may own 
//! the mutex. If this rule is not satisfied then it may cause priority 
//! inversion problems.
//! @return FX_MUTEX_OK in case of success, error code otherwise.
//!
int 
fx_mutex_init(
    fx_mutex_t* mutex, 
    unsigned int priority, 
    fx_sync_policy_t policy)
{
    lang_param_assert(mutex != NULL, FX_MUTEX_INVALID_PTR);
    lang_param_assert(
        priority == FX_MUTEX_CEILING_DISABLED || 
            priority < FX_SCHED_ALG_PRIO_NUM - 1, 
        FX_MUTEX_INVALID_PRIORITY
    );
    lang_param_assert(policy < FX_SYNC_POLICY_MAX, FX_MUTEX_UNSUPPORTED_POLICY);

    fx_spl_spinlock_init(&mutex->lock);
    fx_sync_waitable_init(
        &mutex->waitable, 
        &mutex->lock, 
        fx_mutex_test_and_wait
    );
    mutex->policy = policy;
    mutex->recursive_locks = 0;
    mutex->owner = NULL;
    fx_rtp_init(&mutex->rtp, FX_MUTEX_MAGIC);

    if (priority != FX_MUTEX_CEILING_DISABLED)
    {
        mutex->ceiling_enabled = true; 
        fx_sched_params_init_prio(&mutex->ceiling_orig, priority);
        fx_sched_params_init(
            &mutex->owner_params, 
            FX_SCHED_PARAMS_INIT_DEFAULT, 
            NULL
        );
    }
    else
    {
        mutex->ceiling_enabled = false;
    }

    trace_mutex_init(&mutex->trace_handle);
    return FX_MUTEX_OK;
}

//!
//! Mutex destructor.
//! @param [in,out] mutex Mutex object to be destructed.
//! @return FX_MUTEX_OK in case of success, error code otherwise.
//! @remark All waiters will be released with status FX_WAIT_DELETED. 
//!
int 
fx_mutex_deinit(fx_mutex_t* mutex)
{
    fx_sched_state_t prev;
    lang_param_assert(mutex != NULL, FX_MUTEX_INVALID_PTR);
    lang_param_assert(fx_mutex_is_valid(mutex), FX_MUTEX_INVALID_OBJ);
        
    fx_sched_lock(&prev);
    fx_rtp_deinit(&mutex->rtp);
    fx_sync_waitable_lock(&mutex->waitable);
    _fx_sync_wait_notify(&mutex->waitable, FX_WAIT_DELETED, NULL);
    fx_sync_waitable_unlock(&mutex->waitable);

    //
    // If mutex is not free and ceiling enabled - perform un-ceiling.
    //
    if (mutex->owner != NULL && mutex->ceiling_enabled)
    {
        fx_thread_lock(mutex->owner);
        fx_sched_item_set_params(
            fx_thread_as_sched_item(mutex->owner), 
            &mutex->owner_params
        );
        fx_thread_unlock(mutex->owner);
    }
    fx_sched_unlock(prev);
    trace_mutex_deinit(&mutex->trace_handle);
    return FX_MUTEX_OK;
}

//!
//! Mutex acquiring with cancel event.
//! @param [in] mutex Mutex to be acquired.
//! @param [in] abort_event Event object which is used to cancel waiting.
//! @return Wait status.
//! @remark Mutex state is not changed if function return non-OK status.
//!
int 
fx_mutex_acquire(fx_mutex_t* mutex, fx_event_t* abort_event)
{
    lang_param_assert(mutex != NULL, FX_MUTEX_INVALID_PTR);
    lang_param_assert(fx_mutex_is_valid(mutex), FX_MUTEX_INVALID_OBJ);

    fx_dbg_assert(mutex->recursive_locks < UINT_FAST16_MAX);
    return fx_thread_wait_object(&mutex->waitable, NULL, abort_event);
}

//!
//! Mutex acquiring with timeout.
//! @param [in] mutex Mutex to be acquired.
//! @param [in] timeout Acquiring timeout (in ticks).
//! @return Wait status.
//! @remark Mutex state is not changed if function return non-OK status.
//!
int 
fx_mutex_timedacquire(fx_mutex_t* mutex, uint32_t timeout)
{
    lang_param_assert(mutex != NULL, FX_MUTEX_INVALID_PTR);
    lang_param_assert(fx_mutex_is_valid(mutex), FX_MUTEX_INVALID_OBJ);

    fx_dbg_assert(mutex->recursive_locks < UINT_FAST16_MAX);
    return fx_thread_timedwait_object(&mutex->waitable, NULL, timeout);
}

//!
//! Mutex releasing.
//! @param [in,out] mutex Mutex object to be released.
//! @param [in] policy Waiter releasing policy.
//! @return FX_STATUS_OK in case of success, error code otherwise.
//! @warning In SMP systems with ceiling enabled this function may have to 
//! acquire two nested spinlocks. It may affect real-time performance in case 
//! of active using of mutexes in system.
//!
int 
fx_mutex_release_with_policy(fx_mutex_t* mutex, fx_sync_policy_t policy)
{
    fx_thread_t* me  = fx_thread_self();
    fx_sched_state_t prev;
    
    //
    // Parameters assertions. 
    // N.B. Only owner can release mutex, otherwise FX_STATUS_INVALID_OPERATION 
    // will be returned. 
    //
    lang_param_assert(mutex != NULL, FX_MUTEX_INVALID_PTR);
    lang_param_assert(mutex->owner == me, FX_MUTEX_WRONG_OWNER);
    lang_param_assert(fx_mutex_is_valid(mutex), FX_MUTEX_INVALID_OBJ);
    lang_param_assert(policy < FX_SYNC_POLICY_MAX, FX_MUTEX_UNSUPPORTED_POLICY);

    fx_sched_lock(&prev);
    fx_sync_waitable_lock(&mutex->waitable);

    if (mutex->recursive_locks == 0)
    {
        fx_sched_params_t sched_params;

        //
        // Save original scheduling parameters from mutex to local storage. 
        // If mutex has waiter these paramters will be overwritten by parameters 
        // of new owner. After ceiling has performed restore original scheduling
        // parameters to current thread.
        //
        fx_sched_params_copy(&mutex->owner_params, &sched_params);

        //
        // If mutex has waiter: get it and perform ceiling. This action must be 
        // performed inside mutex lock because there is no guarantee that waiter 
        // won't skip wait by any other reason. In this case ceiling can be
        // performed incorrectly.
        //
        if (_fx_sync_waitable_nonempty(&mutex->waitable))
        {
            fx_sync_wait_block_t* wb = _fx_sync_wait_block_get(
                &mutex->waitable, 
                policy
            );
            fx_sync_waiter_t* waiter = wb->waiter;

            mutex->owner = lang_containing_record(waiter, fx_thread_t, waiter);

            //
            // If ceiling is enabled copy scheduling parameters of current 
            // thread to temporary storage and apply ceiling priority to the 
            // new owner.
            //
            if (mutex->ceiling_enabled)
            {
                fx_sched_params_copy(
                    fx_thread_as_sched_params(mutex->owner), 
                    &mutex->owner_params
                );

                if (fx_sched_params_is_preempt(
                        &mutex->ceiling_orig, 
                        fx_thread_as_sched_params(mutex->owner)))
                {
                    fx_thread_lock(mutex->owner);
                    fx_sched_item_set_params(
                        fx_thread_as_sched_item(mutex->owner), 
                        &mutex->ceiling_orig
                    );
                    fx_thread_unlock(mutex->owner);

                    trace_thread_ceiling(
                        fx_thread_as_trace_handle(mutex->owner), 
                        fx_sched_params_as_number(&mutex->owner_params),
                        fx_sched_params_as_number(&mutex->ceiling_orig)
                    );
                }
            }

            _fx_sync_wait_notify(&mutex->waitable, FX_WAIT_SATISFIED, wb);
            trace_mutex_released(
                &mutex->trace_handle, 
                fx_thread_as_trace_handle(mutex->owner)
            );
        }
        else
        {
            mutex->owner = NULL;
            trace_mutex_released(&mutex->trace_handle, NULL);
        }  

        //
        // Restore scheduling parameters of thread that released the mutex.
        //
        if (mutex->ceiling_enabled)
        {
            if (!fx_sched_params_is_equal(
                    &sched_params, 
                    fx_thread_as_sched_params(me)))
            {
                fx_thread_lock(me);
                fx_sched_item_set_params(
                    fx_thread_as_sched_item(me), 
                    &sched_params
                );
                fx_thread_unlock(me);
            }

            trace_thread_deceiling(
                fx_thread_as_trace_handle(me), 
                fx_sched_params_as_number(&mutex->ceiling_orig),
                fx_sched_params_as_number(&sched_params)
            );
        }
    }
    else
    {
        --mutex->recursive_locks;
    }

    fx_sync_waitable_unlock(&mutex->waitable);
    fx_sched_unlock(prev);
    return FX_MUTEX_OK;
}

//!
//! Mutex release.
//! @param [in,out] mutex Mutex object to be released.
//! @return FX_MUTEX_OK in case of success, error code otherwise.
//!
int 
fx_mutex_release(fx_mutex_t* mutex)
{
    return fx_mutex_release_with_policy(mutex, mutex->policy);
}

//!
//! Gets owner of the mutex.
//! @param [in,out] mutex Mutex object.
//! @return Owner thread or NULL if mutex is not acquired.
//!
fx_thread_t*          
fx_mutex_get_owner(fx_mutex_t* mutex)
{
    lang_param_assert(mutex != NULL, NULL);
    lang_param_assert(fx_mutex_is_valid(mutex), NULL);
    
    return mutex->owner;
}
