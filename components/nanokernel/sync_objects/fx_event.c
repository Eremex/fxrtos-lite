/** 
  ******************************************************************************
  *  @file   fx_event.c
  *  @brief  Implementation of simple events.
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

#include FX_INTERFACE(FX_EVENT)
#include FX_INTERFACE(HW_CPU)

FX_METADATA(({ implementation: [FX_EVENT, V1] }))

//!
//! Initializes the event without parameter checking and object validation.
//! @param [in,out] event Event object to be initialized.
//! @param [in] state Initial state of event.
//! @warning Internal use only!
//!
void 
fx_event_internal_init(fx_event_internal_t* event, const bool state)
{
    fx_spl_spinlock_init(&event->lock);
    fx_sync_waitable_init(&event->waitable,&event->lock,fx_event_test_and_wait);
    event->state = state;
}

//!
//! Setting of the event without parameter checking and object validation.
//! It releases only waiters that have been inserted in the event's queue before
//! this call.
//! @param [in,out] event Event object to be set.
//!
void 
fx_event_internal_set(fx_event_internal_t* event)
{
    fx_sched_state_t prev;
    fx_sched_lock(&prev);
    fx_sync_waitable_lock(&event->waitable);
    {
        const bool old_state = event->state;
        event->state = true;

        if (old_state == false)
        {
            _fx_sync_wait_notify(&event->waitable, FX_WAIT_SATISFIED, NULL);
        }
    }
    fx_sync_waitable_unlock(&event->waitable);
    fx_sched_unlock(prev);
}

//!
//! Resetting of the event without parameter checking and object validation.
//! Event will become nonsignaled after this call. If it is already nonsignaled 
//! no action will be performed.
//! @param [in,out] event Event object to be reset.
//!
void 
fx_event_internal_reset(fx_event_internal_t* event)
{
    fx_sched_state_t prev;
    fx_sched_lock(&prev);
    fx_sync_waitable_lock(&event->waitable);
    event->state = false;
    fx_sync_waitable_unlock(&event->waitable);
    fx_sched_unlock(prev);
}

//!
//! Initializes the event.
//! @param [in,out] event Event object to be initialized.
//! @param [in] state Initial state of event.
//! @return FX_EVENT_OK in case of successful initialization, error code 
//! otherwise.
//! @sa fx_event_deinit
//!
int 
fx_event_init(fx_event_t* event, const bool state)
{
    lang_param_assert(event != NULL, FX_EVENT_INVALID_PTR);

    fx_rtp_init(&event->rtp, FX_EVENT_MAGIC);
    fx_event_internal_init(&event->object, state);
    return FX_EVENT_OK;
}

//!
//! Destructor of the event.
//! @param [in,out] event Event object to be deinitialized.
//! @return FX_EVENT_OK in case of successful destruction, error code otherwise.
//! @warning  This destructor only releases all waiters with appropriate status.
//! It is not responsible for preventing event use after destruction.
//! @sa fx_event_init
//!
int 
fx_event_deinit(fx_event_t* event)
{
    fx_event_internal_t* internal_event = &event->object;
    fx_sched_state_t prev;    
    lang_param_assert(event != NULL, FX_EVENT_INVALID_PTR);
    lang_param_assert(fx_event_is_valid(event), FX_EVENT_INVALID_OBJ);

    fx_sched_lock(&prev);
    fx_rtp_deinit(&event->rtp);
    fx_sync_waitable_lock(&internal_event->waitable);
    _fx_sync_wait_notify(&internal_event->waitable, FX_WAIT_DELETED, NULL);
    fx_sync_waitable_unlock(&internal_event->waitable);
    fx_sched_unlock(prev);
    return FX_EVENT_OK;
}

//!
//! Set the event to signaled state.
//! It releases only waiters that have been inserted in the event's queue 
//! before this call.
//! @param [in,out] event Event object to be set.
//! @return FX_EVENT_OK in case of success, error code otherwise.
//! @sa fx_event_reset
//!
int  
fx_event_set(fx_event_t* event)
{
    lang_param_assert(event != NULL, FX_EVENT_INVALID_PTR);
    lang_param_assert(fx_event_is_valid(event), FX_EVENT_INVALID_OBJ);

    fx_event_internal_set(&event->object);
    return FX_EVENT_OK;
}

//!
//! Set the event to nonsignaled state.
//! Event will become nonsignaled after this call. If it is already nonsignaled 
//! no action will be performed.
//! @param [in,out] event Event object to be reset.
//! @return FX_EVENT_OK in case of success, error code otherwise.
//! @sa fx_event_set
//!
int  
fx_event_reset(fx_event_t* event)
{
    lang_param_assert(event != NULL, FX_EVENT_INVALID_PTR);
    lang_param_assert(fx_event_is_valid(event), FX_EVENT_INVALID_OBJ);

    fx_event_internal_reset(&event->object);
    return FX_EVENT_OK;
}

//!
//! Atomic test and wait function.
//! It is used for wait implementation and is not an event method.
//! @param [in] object Event object to be tested.
//! @param [in] wb Wait block to be inserted into event's waiter queue if event 
//! is nonsignaled.
//! @param [in] wait Wait option used to test object, if it is nonzero wait will 
//! actually start waiting in case when the event is not active, if it is zero - 
//! this function just tests the object and returns status.
//! @return true in case of object was signaled, false otherwise. If object was 
//! nonsignaled it means that wait block was inserted into event's queue.
//! @remark SPL = SCHED_LEVEL
//!
bool 
fx_event_test_and_wait(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait)
{
    bool event_state;
    fx_event_internal_t* internal_event = lang_containing_record(
        object, 
        fx_event_internal_t, 
        waitable
    );
    fx_sync_waitable_lock(object);
    event_state = internal_event->state;

    if (event_state == false && wait)
    {
        _fx_sync_wait_start(object, wb);
    }
    fx_sync_waitable_unlock(object);
    return event_state;
}

//!
//! Gets state of the event.
//! @param [in,out] event Event object.
//! @param [in] state Pointer to bool variable to save the state to.
//! @return FX_EVENT_OK in case of success, error code otherwise.
//! @sa fx_event_set
//! @sa fx_event_reset
//!
int 
fx_event_get_state(fx_event_t* event, bool* state)
{
    lang_param_assert(event != NULL, FX_EVENT_INVALID_PTR);
    lang_param_assert(state != NULL, FX_EVENT_INVALID_PTR);
    lang_param_assert(fx_event_is_valid(event), FX_EVENT_INVALID_OBJ);
    {
        fx_event_internal_t* internal_event = &event->object;
        *state = internal_event->state;
    }
    return FX_EVENT_OK;
}
