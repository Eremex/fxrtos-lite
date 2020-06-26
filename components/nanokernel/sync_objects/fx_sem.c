/** 
  ******************************************************************************
  *  @file   fx_sem.c
  *  @brief  Implementation of semaphores.
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

#include FX_INTERFACE(FX_SEM)
#include FX_INTERFACE(HW_CPU)

FX_METADATA(({ implementation: [FX_SEM, V1] }))

#define fx_sem_is_valid(sem) (fx_rtp_check((&((sem)->rtp)), FX_SEM_MAGIC))

//!
//! Test and wait function.
//! @param [in] object Semaphore object to be tested.
//! @param [in] wb Wait block to be inserted into semaphore's waiters queue if 
//! object is nonsignaled.
//! @param [in] wait Wait option used to test object, if it is nonzero wait will 
//! actually start in case when the semaphore is 0, if it is zero - this 
//! function just tests the object and return status. 
//! @return true in case of object was signaled (and semaphore is successfully 
//! decremented), false otherwise. If object was nonsignaled it means that wait 
//! block was inserted into semaphore's queue.
//! @remark SPL = SCHED_LEVEL
//!
static bool 
fx_sem_test_and_wait(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait)
{
    fx_sem_t* sem_obj = lang_containing_record(object, fx_sem_t, waitable);
    bool wait_satisfied = false;

    fx_sync_waitable_lock(object);

    if (sem_obj->semaphore > 0)
    {
        --sem_obj->semaphore;
        wait_satisfied = true;
        trace_sem_wait_ok(&sem_obj->trace_handle, sem_obj->semaphore);    
    }
    else if (wait)
    {
        _fx_sync_wait_start(object, wb);

        trace_sem_wait_block(
            &sem_obj->trace_handle, 
            fx_thread_as_trace_handle(
                lang_containing_record(wb->waiter, fx_thread_t, waiter)
            )
        );
    }
    
    fx_sync_waitable_unlock(object);
    return wait_satisfied;
}

//!
//! Initializes the semaphore.
//! @param [in,out] sem Semaphore object to be initialized.
//! @param [in] init Initial value of semaphore counter.
//! @param [in] max_val Maximal value of semaphore counter.
//! @param [in] policy Waiter releasing policy.
//! @return FX_SEM_OK in case of successful initialization, error code 
//! otherwise.
//! @sa fx_sem_deinit
//!
int 
fx_sem_init(
    fx_sem_t* sem, 
    unsigned int init, 
    unsigned int max_val, 
    fx_sync_policy_t policy)
{
    lang_param_assert(sem != NULL, FX_SEM_INVALID_PTR);
    lang_param_assert(init <= max_val, FX_SEM_INVALID_VALUE);
    lang_param_assert(policy < FX_SYNC_POLICY_MAX, FX_SEM_UNSUPPORTED_POLICY);

    fx_spl_spinlock_init(&sem->lock);
    fx_sync_waitable_init(&sem->waitable, &sem->lock, fx_sem_test_and_wait);
    fx_rtp_init(&sem->rtp, FX_SEM_MAGIC);
    sem->semaphore = init;
    sem->max_count = max_val;
    sem->policy = policy;
    trace_sem_init(&sem->trace_handle, init, max_val);
    return FX_SEM_OK;
}

//!
//! Destructor of the semaphore.
//! @param [in,out] sem Semaphore object to be deinitialized.
//! @return FX_STATUS_OK in case of successful destruction, error code otherwise
//! @warning  This destructor only releases all waiters with appropriate status.
//! It is not responsible for preventing of using semaphore after destruction.
//! @sa fx_sem_init
//!
int 
fx_sem_deinit(fx_sem_t* sem)
{
    fx_sched_state_t prev;
    lang_param_assert(sem != NULL, FX_SEM_INVALID_PTR);
    lang_param_assert(fx_sem_is_valid(sem), FX_SEM_INVALID_OBJ);
        
    fx_sched_lock(&prev);
    fx_rtp_deinit(&sem->rtp);
    fx_sync_waitable_lock(&sem->waitable);
    if (!sem->semaphore)
    {
        _fx_sync_wait_notify(&sem->waitable, FX_WAIT_DELETED, NULL);
    }
    fx_sync_waitable_unlock(&sem->waitable);
    fx_sched_unlock(prev);
    trace_sem_deinit(&sem->trace_handle, init, max_val);
    return FX_SEM_OK;
}

//!
//! Reset the semaphore counter.
//! Semaphore counter will be set to zero.
//! @param [in,out] sem Semaphore object to be reset.
//! @return FX_STATUS_OK in case of success, error code otherwise.
//! @sa fx_sem_post
//!
int 
fx_sem_reset(fx_sem_t* sem)
{
    fx_sched_state_t prev;
    lang_param_assert(sem != NULL, FX_SEM_INVALID_PTR);
    lang_param_assert(fx_sem_is_valid(sem), FX_SEM_INVALID_OBJ);
        
    fx_sched_lock(&prev);
    fx_sync_waitable_lock(&sem->waitable);
    sem->semaphore = 0;
    fx_sync_waitable_unlock(&sem->waitable);
    fx_sched_unlock(prev);
    return FX_SEM_OK;
}

//!
//! Post semaphore.
//! Increment semaphore counter. It will release waiting threads 
//! (while counter is nonzero).
//! @param [in] sem Semaphore object to be incremented.
//! @param [in] p Waiter releasing policy.
//! @return FX_SEM_OK in case of success, error code otherwise.
//! @sa fx_sem_init
//! @sa fx_sem_reset
//! @sa fx_sem_wait
//!
int 
fx_sem_post_with_policy(fx_sem_t* sem, fx_sync_policy_t p)
{
    fx_sched_state_t prev;
    lang_param_assert(sem != NULL, FX_SEM_INVALID_PTR);
    lang_param_assert(fx_sem_is_valid(sem), FX_SEM_INVALID_OBJ);
    lang_param_assert(p < FX_SYNC_POLICY_MAX, FX_SEM_UNSUPPORTED_POLICY);
        
    fx_sched_lock(&prev);
    fx_sync_waitable_lock(&sem->waitable);
    if (_fx_sync_waitable_nonempty(&sem->waitable)) 
    {
        fx_sync_wait_block_t* wb = _fx_sync_wait_block_get(&sem->waitable, p);
        _fx_sync_wait_notify(&sem->waitable, FX_WAIT_SATISFIED, wb);
    }
    else if (sem->semaphore < sem->max_count)
    {
        ++sem->semaphore;
    }
    trace_sem_post(&sem->trace_handle, sem->semaphore);
    fx_sync_waitable_unlock(&sem->waitable);
    fx_sched_unlock(prev);
    return FX_SEM_OK;
}

//!
//! Post semaphore.
//! Increment semaphore counter. It will release waiting threads 
//! (while counter is nonzero).
//! @param [in] sem Semaphore object to be incremented.
//! @return FX_SEM_OK in case of success, error code otherwise.
//! @sa fx_sem_init
//! @sa fx_sem_reset
//! @sa fx_sem_wait
//!
int 
fx_sem_post(fx_sem_t* sem)
{
    return fx_sem_post_with_policy(sem, sem->policy);
}

//!
//! Try to decrement semaphore with cancel event.
//! @param [in] sem Semaphore to wait.
//! @param [in] abort_event Event object which is used to cancel waiting.
//! @return Wait status.
//! @remark Semaphore state is not changed if function return non-OK status.
//!
int 
fx_sem_wait(fx_sem_t* sem, fx_event_t* abort_event)
{
    lang_param_assert(sem != NULL, FX_SEM_INVALID_PTR);
    lang_param_assert(fx_sem_is_valid(sem), FX_SEM_INVALID_OBJ);

    return fx_thread_wait_object(&sem->waitable, NULL, abort_event);
}

//!
//! Try to decrement semaphore with timeout.
//! @param [in] sem Semaphore to wait.
//! @param [in] timeout Wait timeout (in milliseconds).
//! @return Wait status.
//! @remark Semaphore state is not changed if function return non-OK status.
//!
int 
fx_sem_timedwait(fx_sem_t* sem, uint32_t timeout)
{
    lang_param_assert(sem != NULL, FX_SEM_INVALID_PTR);
    lang_param_assert(fx_sem_is_valid(sem), FX_SEM_INVALID_OBJ);

    return fx_thread_timedwait_object(&sem->waitable, NULL, timeout);
}

//!
//! Get current value of the semaphore.
//! @param [in] sem Semaphore.
//! @param [in] value Pointer where current semaphore value will be stored.
//! @return FX_SEM_OK in case of success, error code otherwise.
//!
int         
fx_sem_get_value(fx_sem_t* sem, unsigned int* value)
{
    lang_param_assert(sem != NULL, FX_SEM_INVALID_PTR);
    lang_param_assert(value != NULL, FX_SEM_INVALID_PTR);
    lang_param_assert(fx_sem_is_valid(sem), FX_SEM_INVALID_OBJ);
    
    *value = sem->semaphore;
    return FX_SEM_OK;
}
