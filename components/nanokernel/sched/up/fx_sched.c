/**
  ******************************************************************************
  *  @file   fx_sched.c
  *  @brief  Implementation of global uniprocessor scheduler.
  *  Scheduling context contains counter which is incremented on every change
  *  of underlying container. This design allows to boost thread priorities by
  *  priority inheritance/ceiling mechanism without global rescheduling.
  *  when the priority is raised no rescheduling needed since we stay the most
  *  prioritized item. On lowering if the changes counter value is the same 
  *  then no thread may appear between our current and old priority, so, we
  *  may safely drop priority without rescheduling again.
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

#include FX_INTERFACE(FX_SCHED)
#include FX_INTERFACE(HW_CPU)
#include FX_INTERFACE(HAL_ASYNC)
#include FX_INTERFACE(HAL_MP)
#include FX_INTERFACE(FX_DPC)

FX_METADATA(({ implementation: [FX_SCHED, UP_FIFO] }))
lang_static_assert(HAL_MP_CPU_MAX == 1);

//!
//! Global scheduler context. It holds all active threads existing in the system
//!
typedef struct
{
    bool resched_pending;           //!< If the flag is set: resched is pending.
    unsigned int changes_counter;   //!< Counts changes of scheduling state.
    fx_sched_container_t g_domain;  //!< Container of executable entities.
    fx_sched_item_t* active;        //!< Currently active item.
}
fx_sched_context_t;

fx_sched_context_t global_domain;
#define fx_sched_get_context() (&global_domain)

//!
//! Constructor of global scheduler module.
//! It should be called on each processor in the system.
//!
void
fx_sched_ctor(void)
{
    fx_sched_context_t* context = fx_sched_get_context();
    fx_sched_container_init(&(context->g_domain));
}

//!
//! Rescheduling request.
//!
void
fx_sched_mark_resched_needed(void)
{
    fx_sched_context_t* context = fx_sched_get_context();

    context->resched_pending = true;

    //
    // Request dispatch software interrupt.
    // If we're in DPC environment then scheduler will be called unconditionally
    // after handling of the DPC queue, so, software interrupt may be skipped.
    //
    if (!fx_dpc_environment())
    {
        hal_async_request_swi(SPL_DISPATCH);
    }
}

//!
//! Initialization of scheduling entity.
//! Items always initialized as suspended. Use @ref fx_sched_item_resume to make
//! it active.
//! @param [in,out] item Scheduling entity to be initialized.
//! @param [in] t Initialization type. See @ref fx_sched_params_init for details
//! @param [in] arg Source of scheduling parameters in case of initialization 
//! type is FX_SCHED_PARAMS_INIT_SPECIFIED.
//! @sa fx_sched_item_add
//!
void
fx_sched_item_init(
    fx_sched_item_t* item, 
    fx_sched_params_init_t t, 
    const fx_sched_params_t* arg)
{
    fx_sched_params_init( fx_sched_item_as_sched_params(item), t, arg);

    //
    // Initialized items always suspended.
    //
    item->suspend_count = 1;

    //
    // It is special case, when initialization performed on IDLE thread. 
    // User never uses IDLE directly it is called only during system 
    // initialization. Since rescheduling is requested only when new thread 
    // preempts current one, there's need to set IDLE as currently active item.
    //
    if (t == FX_SCHED_PARAMS_INIT_IDLE)
    {
        fx_sched_context_t* context = fx_sched_get_context();
        context->active = item;
    }
}

//!
//! Removing scheduling entity from the scheduler.
//! @param [in] item Initialized scheduling entity to be removed.
//! @sa fx_sched_item_init
//! @sa fx_sched_item_add
//!
void
fx_sched_item_remove(fx_sched_item_t* item)
{
    if (item->suspend_count == 0)
    {
        fx_sched_context_t* context = fx_sched_get_context();
        fx_sched_container_remove(
            &context->g_domain, 
            fx_sched_item_as_sched_params(item)
        );

        ++context->changes_counter;

        if (context->active == item)
        {
            fx_sched_mark_resched_needed();
        }
    }
}

//!
//! Setting scheduling parameters for schedulable entity.
//! @param [in] item Scheduling entity in which parameters should be changed.
//! @param [in] src Scheduling parameters to be copied into target.
//! @sa fx_sched_item_get_params
//!
void
fx_sched_item_set_params(fx_sched_item_t* item, const fx_sched_params_t* src)
{
    //
    // Parameters changing may be performed only on item that is not in 
    // the container. If scheduling entity is active now it is needed to be 
    // re-added into container to rebuild container's internal structures.
    //
    if (item->suspend_count == 0)
    {
        fx_sched_context_t* context = fx_sched_get_context();

        const bool raising_prio = fx_sched_params_is_preempt(
            src, 
            fx_sched_item_as_sched_params(item)
        );

        const bool self = (item == context->active);
        static fx_sched_params_t saved_params = { 0 };
        static unsigned int timestamp = 0;

        if (self && raising_prio)
        {
            timestamp = context->changes_counter;
            fx_sched_params_copy(
                fx_sched_item_as_sched_params(item), 
                &saved_params
            );
        }
        else if (
            !self || 
            context->changes_counter != timestamp || 
            !fx_sched_params_is_equal(&saved_params, src))
        {
            ++context->changes_counter;
            fx_sched_mark_resched_needed();            
        }

        fx_sched_container_remove(
            &context->g_domain, 
            fx_sched_item_as_sched_params(item)
        );
        fx_sched_params_copy(src, fx_sched_item_as_sched_params(item));
        fx_sched_container_add(
            &context->g_domain, 
            fx_sched_item_as_sched_params(item)
        );
    }
    else
    {
        //
        // If item is suspended (it means it is not resides in the scheduler) 
        // then parameters may be just copied into target.
        //
        fx_sched_params_copy(src, fx_sched_item_as_sched_params(item));
    }
}

//!
//! Returns scheduling parameters of schedulable entity.
//! @param [in] src Source of scheduling params.
//! @param [in,out] dst Placeholder of scheduling parameters extracted from 
//! source item.
//!
void 
fx_sched_item_get_params(fx_sched_item_t* src, fx_sched_params_t* dst)
{
    fx_sched_params_copy(fx_sched_item_as_sched_params(src), dst);
}

//!
//! Suspends schedulable entity.
//! @param [in] item Scheduling entity to be suspended.
//! @return Previous value of suspend counter.
//! @sa fx_sched_item_resume
//! @warning This may be called only by entity itself. 
//!
unsigned int
fx_sched_item_suspend(fx_sched_item_t* item)
{
    const unsigned int prev_suspend_count = item->suspend_count;

    //
    // If item became inactive (suspend counter was being changed from 0 to 1) 
    // then remove it from the container.
    //
    if (++(item->suspend_count) == 1)
    {
        fx_sched_context_t* context = fx_sched_get_context();
        fx_sched_container_remove(
            &context->g_domain, 
            fx_sched_item_as_sched_params(item)
        );
        ++context->changes_counter;
        fx_sched_mark_resched_needed();
    }

    return prev_suspend_count;
}

//!
//! Resumes schedulable entity.
//! @param [in] item Scheduling entity to be resumed.
//! @return Previous value of suspend counter.
//! @sa fx_sched_item_suspend
//!
unsigned int
fx_sched_item_resume(fx_sched_item_t* item)
{
    const unsigned int prev_suspend_count = item->suspend_count;

    //
    // If item is active (suspend counter is being changed from 1 to 0) 
    // then add it to the container.
    //
    if (prev_suspend_count > 0 && --(item->suspend_count) == 0)
    {
        fx_sched_context_t* context = fx_sched_get_context();
        fx_sched_container_add(
            &context->g_domain, 
            fx_sched_item_as_sched_params(item)
        );

        ++context->changes_counter;

        if (fx_sched_params_is_equal(
                fx_sched_item_as_sched_params(item),
                fx_sched_item_as_sched_params(context->active)) ||
            fx_sched_params_is_preempt(
                fx_sched_item_as_sched_params(item), 
                fx_sched_item_as_sched_params(context->active)))
        {
            fx_sched_mark_resched_needed();
        }
    }

    return prev_suspend_count;
}

//!
//! It uses by dispatch handler to select next schedulable entity to execute.
//! @return Scheduled entity. If this function returns NULL it means that 
//! entity is not changed since last rescheduling.
//!
fx_sched_item_t*
fx_sched_get_next(void)
{
    fx_sched_context_t* context = fx_sched_get_context();
    fx_sched_item_t* next = NULL;

    if (context->resched_pending == 1)
    {
        fx_sched_params_t* item = fx_sched_container_get(&context->g_domain);
        next = lang_containing_record(item, fx_sched_item_t, sched_params);
        context->resched_pending = false; 
        context->active = next;
    }

    return next;
}

//!
//! Placing specified item at end of queue.
//!
bool
fx_sched_yield(fx_sched_item_t* item)
{
    bool result = false;
    fx_sched_context_t* context = fx_sched_get_context();

    if (item->suspend_count == 0)
    {
        fx_sched_container_remove(
            &context->g_domain, 
            fx_sched_item_as_sched_params(item)
        );
        fx_sched_container_add(
            &context->g_domain, 
            fx_sched_item_as_sched_params(item)
        );
        result = true;
        ++context->changes_counter;
        fx_sched_mark_resched_needed();
    }
    return result;
}
