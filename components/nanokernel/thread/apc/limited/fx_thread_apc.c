/** 
  ******************************************************************************
  *  @file   fx_thread_apc.c
  *  @brief  Implementation of limited APCs.
  *  Design note: Normally thread termination dispatching is performed as 
  *  APC procedure executing in context of the target thread. This is neccessary
  *  for multiprocessor systems in order to synchronize with the thread 
  *  execution. In uniprocessor systems termination APC may be implemented in 
  *  more efficient way: if target function (termination handler) is SPL 
  *  agnostic (may be called at both LOW and DISPATCH levels) it may be directly
  *  called instead of sending APC message. APC queuing may be performed at SPL
  *  = DISPATCH only, so, in uniprocessor system we're already synchronized with
  *  all threads in the system. This implementation is intended to be used in 
  *  simple configurations with kernel only (without executive subsystem). It 
  *  does NOT support APC masking, deferred delivery, custom APC messages, etc.
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

#include FX_INTERFACE(FX_THREAD_APC)
#include FX_INTERFACE(FX_DPC)
#include FX_INTERFACE(FX_PROCESS)
#include FX_INTERFACE(HAL_MP)
#include FX_INTERFACE(FX_EVENT)

FX_METADATA(({ implementation: [FX_THREAD_APC, LIMITED] }))

lang_static_assert(HAL_MP_CPU_MAX == 1);

void (*fx_thread_apc_on_receive)(fx_thread_apc_target_t*);

typedef struct
{
    fx_thread_apc_target_t* target;
    fx_event_internal_t* completion_event;
}
fx_thread_apc_info_t;

static void 
fx_thread_apc_helper_dpc(fx_dpc_t* object, void* arg)
{
    fx_thread_apc_info_t* const info = arg;
    fx_thread_apc_target_t* const target = info->target;
    const fx_process_exception_handler_t handler = 
        fx_process_get_exception(FX_EXCEPTION_TERM);

    //
    // Cancel any side-effects like timers and waits.
    //
    fx_thread_apc_on_receive(target);

    //
    // Call termination handler. It MUST use argument instead of 
    // "get_current_thread" since it may be called in context of any thread.
    //
    handler(target, FX_EXCEPTION_TERM, NULL);

    //
    // Thread's completion event normally should be set by the scheduler when 
    // it switches from thread in "completed state". On SMP systems threads may 
    // be killed only via APC and it is guaranteed that the event will always be
    // set. However, in this simplified uniprocessor implementation a thread may
    // kill any other thread, without scheduler transition from killed thread 
    // and completion event therefore may not be set causing join calls to be
    // blocked forever. In order to avoid such behavior we set the event 
    // manually. In case when a threadkills itself, scheduler will also set 
    // the event.
    //
    fx_event_internal_set(info->completion_event);
}

//!
//! Perform thread termination.
//! This function call appropriate exception handler via DPC.
//! @param [in] target Target thread to be terminated.
//! @param [in] reason Unused at now.
//! @param [out] context Unused at now.
//! SPL >= SCHED_LEVEL
//!
bool 
fx_thread_apc_insert_internal(
    fx_thread_apc_target_t* target, 
    unsigned int reason, 
    void* context)
{
    //
    // Since this function may be called only at DISPATCH level or above, 
    // single DPC may be used to kill any thread. 
    //
    static fx_dpc_t dpc;
    static fx_thread_apc_info_t dpc_arg;

    //
    // Note that in unified synchronization scheme DPC requests result in direct
    // function calls.
    //
    fx_dpc_init(&dpc);
    dpc_arg.completion_event = context;
    dpc_arg.target = target;

    (void) fx_dpc_request(&dpc, fx_thread_apc_helper_dpc, &dpc_arg);

    //
    // Return value is used as flag indicating whether the thread should be 
    // made runnable.
    //
    return false;
}
