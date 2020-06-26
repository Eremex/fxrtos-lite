/**
  ******************************************************************************
  *  @file   fx_rwlock.c
  *  @brief  Implementation of reader/writer locks.
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

#include FX_INTERFACE(FX_RWLOCK)

FX_METADATA(({ implementation: [FX_RWLOCK, V1] }))

#define fx_rwlock_is_valid(rwl) (fx_rtp_check((&((rwl)->rtp)), FX_RWLOCK_MAGIC))

//
// Test and wait function for readers. 
// @param [in] object Waitable object to be tested.
// @param [in] wb Wait block to be inserted into queue if RWL is nonsignaled.
// @param [in] wait If true causes the waitblock to be placed into rwlock queue 
// if wait cannot be satisfied immediately. If false - just check the object 
// without placing WB in the queue even when rwlock is busy.
// @return true in case of object is signaled, false otherwise. 
// @remark SPL = SCHED_LEVEL
//
static bool
fx_rwlock_test_and_wait_reader(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait)
{
    fx_rwlock_t* rwlock = lang_containing_record(object, fx_rwlock_t, rd_wtbl);
    bool wait_satisfied = false;

    fx_sync_waitable_lock(object);

    //
    // If rwlock is not acquired by writer and there are no waiting writers.
    //
    if (rwlock->owner==NULL && !_fx_sync_waitable_nonempty(&rwlock->wr_wtbl))
    {
        ++rwlock->readers;
        wait_satisfied = true;
    }
    else if (wait)
    {
        _fx_sync_wait_start(object, wb);
    }

    fx_sync_waitable_unlock(object);
    return wait_satisfied;
}

//
// Test and wait function for writers.
// @param [in] object Waitable object to be tested.
// @param [in] wb Wait block to be inserted into queue if it is nonsignaled.
// @param [in] wait If true causes the waitblock to be placed into rwlock queue 
// if wait cannot be satisfied immediately. If false - just check the object 
// without placing WB in the queue even when rwlock is busy.
// @return true in case of object is signaled, false otherwise. 
// @remark SPL = SCHED_LEVEL
//
static bool
fx_rwlock_test_and_wait_writer(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait)
{
    fx_thread_t* me = fx_thread_self();
    fx_rwlock_t* rwlock = lang_containing_record(object, fx_rwlock_t, wr_wtbl);
    bool wait_satisfied = false;

    fx_sync_waitable_lock(object);

    if (rwlock->owner == NULL && rwlock->readers == 0)
    {
        rwlock->owner = me;
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
//! Rwlock initialization.
//! @param [in,out] rwlock Rwlock object to be initialized.
//! @param [in] policy Waiter releasing policy (for both readers and writers).
//! @return FX_RWLOCK_OK in case of success, error code otherwise.
//! @sa fx_rwlock_deinit
//!
int
fx_rwlock_init(fx_rwlock_t* rwlock, const fx_sync_policy_t policy)
{
    lang_param_assert(rwlock != NULL, FX_RWLOCK_INVALID_PTR);
    lang_param_assert(policy < FX_SYNC_POLICY_MAX,FX_RWLOCK_UNSUPPORTED_POLICY);

    fx_rtp_init(&rwlock->rtp, FX_RWLOCK_MAGIC);
    fx_spl_spinlock_init(&rwlock->lock);
    fx_sync_waitable_init(
        &rwlock->rd_wtbl,
        &rwlock->lock,
        fx_rwlock_test_and_wait_reader
    );
    fx_sync_waitable_init(
        &rwlock->wr_wtbl,
        &rwlock->lock,
        fx_rwlock_test_and_wait_writer
    );
    rwlock->policy = policy;
    return FX_RWLOCK_OK;
}

//!
//! Rwlock destructor.
//! @param [in,out] rwlock Rwlock object to be destructed.
//! @return FX_RWLOCK_OK in case of success, error code otherwise.
//! @remark All waiters will be released with status FX_WAIT_DELETED.
//! @sa fx_rwlock_init
//!
int
fx_rwlock_deinit(fx_rwlock_t* rwlock)
{
    fx_sched_state_t prev;
    lang_param_assert(rwlock != NULL, FX_RWLOCK_INVALID_PTR);
    lang_param_assert(fx_rwlock_is_valid(rwlock), FX_RWLOCK_INVALID_OBJ);
        
    fx_sched_lock(&prev);
    fx_rtp_deinit(&rwlock->rtp);
    fx_sync_waitable_lock(&rwlock->rd_wtbl);
    _fx_sync_wait_notify(&rwlock->rd_wtbl, FX_WAIT_DELETED, NULL);
    _fx_sync_wait_notify(&rwlock->wr_wtbl, FX_WAIT_DELETED, NULL);
    fx_sync_waitable_unlock(&rwlock->rd_wtbl);
    fx_sched_unlock(prev);
    return FX_RWLOCK_OK;
}

//!
//! Unlocking the rwlock.
//! @param [in,out] rwlock Rwlock object to be unlocked.
//! @param [in] policy Waiter releasing policy.
//! @return FX_RWLOCK_OK in case of success, error code otherwise.
//! @sa fx_rwlock_rd_lock
//! @sa fx_rwlock_wr_lock
//!
int
fx_rwlock_unlock_with_policy(fx_rwlock_t* rwlock, fx_sync_policy_t policy)
{
    fx_sched_state_t prev;
    const fx_thread_t* me = fx_thread_self();    
    lang_param_assert(rwlock != NULL, FX_RWLOCK_INVALID_PTR);
    lang_param_assert(fx_rwlock_is_valid(rwlock), FX_RWLOCK_INVALID_OBJ);
    lang_param_assert(policy < FX_SYNC_POLICY_MAX,FX_RWLOCK_UNSUPPORTED_POLICY);

    fx_sched_lock(&prev);
    fx_sync_waitable_lock(&rwlock->rd_wtbl);

    //
    // Release by writer.
    //
    if (rwlock->owner == me)
    {
        //
        // Release next writer if writers queue is not empty.
        //
        if (_fx_sync_waitable_nonempty(&rwlock->wr_wtbl))
        {
            fx_sync_wait_block_t* wb = _fx_sync_wait_block_get(
                &rwlock->wr_wtbl, 
                policy
            );
            fx_sync_waiter_t* waiter = wb->waiter;
            rwlock->owner = lang_containing_record(waiter, fx_thread_t, waiter);
            _fx_sync_wait_notify(&rwlock->wr_wtbl, FX_WAIT_SATISFIED, wb);
        }
        else
        {
            rwlock->owner = NULL;

            //
            // Release readers until some writer is blocked. Notification 
            // function releases the spinlock and queue may be changed after 
            // each iteration.
            //
            while ( _fx_sync_waitable_nonempty(&rwlock->rd_wtbl) &&
                    !_fx_sync_waitable_nonempty(&rwlock->wr_wtbl))
            {
                fx_sync_wait_block_t* wb = _fx_sync_wait_block_get(
                    &rwlock->rd_wtbl, 
                    policy
                );
                ++rwlock->readers;
                _fx_sync_wait_notify(&rwlock->rd_wtbl, FX_WAIT_SATISFIED, wb);
            }
        }
    }
    //
    // Release by reader.
    //
    else
    {
        if (rwlock->owner == NULL && rwlock->readers > 0)
        {
            --rwlock->readers;

            //
            // Notify writer if we are the last reader.
            //
            if (rwlock->readers == 0 && 
                _fx_sync_waitable_nonempty(&rwlock->wr_wtbl))
            {
                fx_sync_wait_block_t* wb = _fx_sync_wait_block_get(
                    &rwlock->wr_wtbl, 
                    policy
                );
                fx_sync_waiter_t* waiter = wb->waiter;
                rwlock->owner = 
                    lang_containing_record(waiter, fx_thread_t, waiter);
                _fx_sync_wait_notify(&rwlock->wr_wtbl, FX_WAIT_SATISFIED, wb);
            }
        }
    }

    fx_sync_waitable_unlock(&rwlock->rd_wtbl);
    fx_sched_unlock(prev);
    return FX_RWLOCK_OK;
}

//!
//! Unlocking the rwlock.
//! @param [in,out] rwlock Rwlock object to be unlocked.
//! @return FX_RWLOCK_OK in case of success, error code otherwise.
//! @sa fx_rwlock_rd_lock
//! @sa fx_rwlock_wr_lock
//!
int
fx_rwlock_unlock(fx_rwlock_t* rwlock)
{
    return fx_rwlock_unlock_with_policy(rwlock, rwlock->policy);
}

//!
//! Locking the rwlock by reader.
//! @param [in] rwlock Rwlock object to be locked (and current thread is a 
//! reader).
//! @param [in] cancel_event Cancel event (wait will be aborted if this event 
//! become signaled during waiting, may be NULL).
//! @return Wait status (See @ref fx_thread_wait_res_t for details).
//! @sa fx_rwlock_unlock
//! @sa fx_rwlock_rd_timedlock
//! @sa fx_rwlock_wr_lock
//! @sa fx_rwlock_wr_timedlock
//!
int
fx_rwlock_rd_lock(fx_rwlock_t* rwlock, fx_event_t* cancel_event)
{
    lang_param_assert(rwlock != NULL, FX_RWLOCK_INVALID_PTR);
    lang_param_assert(fx_rwlock_is_valid(rwlock), FX_RWLOCK_INVALID_OBJ);

    return fx_thread_wait_object(&rwlock->rd_wtbl, NULL, cancel_event);
}

//!
//! Locking the rwlock by reader.
//! @param [in] rwlock Rwlock object to be locked (and current thread is a 
//! reader).
//! @param [in] tout Timeout value (or FX_THREAD_INFINITE_TIMEOUT).
//! @return Wait status (See @ref fx_thread_wait_res_t for details).
//! @sa fx_rwlock_unlock
//! @sa fx_rwlock_rd_lock
//! @sa fx_rwlock_wr_lock
//! @sa fx_rwlock_wr_timedlock
//!
int
fx_rwlock_rd_timedlock(fx_rwlock_t* rwlock, uint32_t tout)
{
    lang_param_assert(rwlock != NULL, FX_RWLOCK_INVALID_PTR);
    lang_param_assert(fx_rwlock_is_valid(rwlock), FX_RWLOCK_INVALID_OBJ);
    lang_param_assert(
        tout < FX_TIMER_MAX_RELATIVE_TIMEOUT, 
        FX_RWLOCK_INVALID_TIMEOUT
    );

    return fx_thread_timedwait_object(&rwlock->rd_wtbl, NULL, tout);
}

//!
//! When writer tries to acquire rwlock, new readers block, even when rwlock is 
//! not locked by any writer (writers has higher priority than readers), if such 
//! writer then cancels waiting i.e. due to timeout, signal, or cancel event, 
//! blocked readers should be unlocked if rwlock is not blocked by writer and 
//! there are no writers in the queue. This function is used in wr-lock 
//! functions in order to release readers, if writer's wait is skipped due to 
//! some reason.
//!
static void
fx_rwlock_kick_readers(fx_rwlock_t* rwlock)
{
    fx_sched_state_t prev;
    fx_sched_lock(&prev);
    fx_sync_waitable_lock(&rwlock->rd_wtbl);

    while (_fx_sync_waitable_nonempty(&rwlock->rd_wtbl) &&
              !_fx_sync_waitable_nonempty(&rwlock->wr_wtbl))
    {
        fx_sync_wait_block_t* wb = _fx_sync_wait_block_get(
            &rwlock->rd_wtbl, 
            rwlock->policy
        );
        ++rwlock->readers;
        _fx_sync_wait_notify(&rwlock->rd_wtbl, FX_WAIT_SATISFIED, wb);
    }

    fx_sync_waitable_unlock(&rwlock->rd_wtbl);
    fx_sched_unlock(prev);
}

//!
//! Locking the rwlock by writer.
//! @param [in] rwlock Rwlock object to be locked (and current thread is a 
//! writer).
//! @param [in] cancel_event Cancel event (wait will be aborted if this event 
//! become signaled during waiting, may be NULL).
//! @return Wait status (See @ref fx_thread_wait_res_t for details).
//! @sa fx_rwlock_unlock
//! @sa fx_rwlock_rd_lock
//! @sa fx_rwlock_rd_timedlock
//! @sa fx_rwlock_wr_timedlock
//!
int
fx_rwlock_wr_lock(fx_rwlock_t* rwlock, fx_event_t* cancel_event)
{
    int res = FX_STATUS_OK;

    lang_param_assert(rwlock != NULL, FX_RWLOCK_INVALID_PTR);
    lang_param_assert(fx_rwlock_is_valid(rwlock), FX_RWLOCK_INVALID_OBJ);

    res = fx_thread_wait_object(&rwlock->wr_wtbl, NULL, cancel_event);
    if (res != FX_STATUS_OK)
    {
        fx_rwlock_kick_readers(rwlock);
    }
    return res;
}

//!
//! Locking the rwlock by writer.
//! @param [in] rwlock Rwlock object to be locked (and current thread is a 
//! writer).
//! @param [in] tout Timeout value (or FX_THREAD_INFINITE_TIMEOUT).
//! @return Wait status (See @ref fx_thread_wait_res_t for details).
//! @sa fx_rwlock_unlock
//! @sa fx_rwlock_rd_lock
//! @sa fx_rwlock_rd_timedlock
//! @sa fx_rwlock_wr_lock
//!
int
fx_rwlock_wr_timedlock(fx_rwlock_t* rwlock, uint32_t tout)
{
    int res = FX_STATUS_OK;
    lang_param_assert(rwlock != NULL, FX_RWLOCK_INVALID_PTR);
    lang_param_assert(fx_rwlock_is_valid(rwlock), FX_RWLOCK_INVALID_OBJ);
    lang_param_assert(
        tout < FX_TIMER_MAX_RELATIVE_TIMEOUT, 
        FX_RWLOCK_INVALID_TIMEOUT
    );

    res = fx_thread_timedwait_object(&rwlock->wr_wtbl, NULL, tout);
    if (res != FX_STATUS_OK)
    {
        //
        // If wait has been skipped kick blocked readers if any.
        //
        fx_rwlock_kick_readers(rwlock);
    }
    return res;
}
