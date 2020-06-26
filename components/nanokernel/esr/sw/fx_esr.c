/** 
  ******************************************************************************
  *  @file   sw/fx_esr.c
  *  @brief  Implementation of event service routines (ESRs).
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
  
#include FX_INTERFACE(FX_ESR)
#include FX_INTERFACE(HW_CPU)
#include FX_INTERFACE(HAL_ASYNC)
#include FX_INTERFACE(HAL_MP)  
#include FX_INTERFACE(FX_DBG) 
#include FX_INTERFACE(FX_DPC)
#include FX_INTERFACE(HAL_CPU_INTR)
#include FX_INTERFACE(FX_TIMER_INTERNAL)
#include <string.h>

FX_METADATA(({ implementation: [FX_ESR, SW] }))

typedef struct
{
    fx_esr_t* volatile current_esr;
    hal_intr_frame_t* volatile virtual_frame;
    fx_esr_t idle_esr;
}
fx_esr_context_t;

static fx_esr_context_t g_esr_context[HAL_MP_CPU_MAX];
#define fx_esr_get_context() (&(g_esr_context[hal_mp_get_current_cpu()]))
#define FX_ESR_MAGIC 0x00C72853
#define fx_esr_is_valid(esr) (fx_rtp_check(&((esr)->rtp), FX_ESR_MAGIC))

//!
//! Default signal handler (it causes ESR to exit in case of exception).
//!
void 
fx_esr_term_handler(fx_esr_exception_target_t* esr, unsigned int sig, void* arg)
{
    fx_esr_exit();
}

//!
//! ESR module contructor. 
//! Initializes the idle ESR.
//! @remark SPL = SYNC.
//!
void
fx_thread_ctor(void)
{
    fx_esr_context_t* const context = fx_esr_get_context();
    fx_esr_t* const esr = &(context->idle_esr);

    fx_dpc_ctor();
    fx_sched_ctor();
    fx_timer_ctor();

    fx_rtp_init(&esr->rtp, FX_ESR_MAGIC);
    fx_sched_item_init(
        &esr->sched_item, 
        FX_SCHED_PARAMS_INIT_IDLE, 
        NULL
    );
    fx_sched_item_add(&esr->sched_item);
    fx_sched_item_resume(&esr->sched_item);

    esr->epl = FX_SCHED_ALG_PRIO_IDLE;
    esr->active = true;
    context->current_esr = esr;

    (void)fx_esr_exception_set_handler(HAL_SIG_SEGV, fx_esr_term_handler, NULL);
    (void)fx_esr_exception_set_handler(HAL_SIG_ILL, fx_esr_term_handler, NULL);
    (void)fx_esr_exception_set_handler(HAL_SIG_TRAP, fx_esr_term_handler, NULL);
    (void)fx_esr_exception_set_handler(HAL_SIG_FPE, fx_esr_term_handler, NULL);

    trace_esr_init_idle(
        &esr->trace_handle, 
        fx_sched_params_as_number(
            fx_sched_item_as_sched_params(&esr->sched_item)
        )
    );
}

//!
//! Initialization of ESR object.
//! ESR is always initialized as suspended. Use @ref fx_esr_activate to make it 
//! active.
//! @param [in,out] esr ESR object to be initialized.
//! @param [in] epl EPL level of this ESR. This level will be transalated to ESR
//! scheduling priority. The level is fixed, to change it ESR must be
//! re-initialized.
//! @param [in] func ESR function to be called when ESR is activated.
//! @param [in] arg Argument of ESR function.
//! @return FX_ESR_OK if succeeded, error code otherwise.
//! @sa fx_esr_deinit
//! @sa fx_esr_activate
//!
int 
fx_esr_init(
    fx_esr_t* esr, 
    epl_t epl, 
    void (*func)(fx_esr_t*, uintptr_t, void*), 
    void* arg)
{
    fx_sched_state_t prev;
    fx_sched_params_t temp;    
    lang_param_assert(esr != NULL, FX_ESR_INVALID_PTR);
    lang_param_assert(epl < FX_SCHED_ALG_PRIO_NUM, FX_ESR_INVALID_EPL);
    lang_param_assert(func != NULL, FX_ESR_INVALID_ENTRY);

    //
    // Use temporary scheduling params object to initialize scheduler parameters
    // encapsulated into ESR object.
    //
    fx_sched_params_init_prio(&temp, epl);
    fx_sched_item_init(
        &esr->sched_item, 
        FX_SCHED_PARAMS_INIT_SPECIFIED, 
        &temp
    );
    fx_sync_waiter_init(
        &esr->waiter, 
        fx_sched_item_as_sched_params(&esr->sched_item)
    );
    fx_rtp_init(&esr->rtp, FX_ESR_MAGIC);
    fx_esr_exception_target_init(&esr->traps);

    esr->func = func;
    esr->arg = arg;
    esr->active = false;
    esr->epl = epl;

    fx_sched_lock(&prev);
    fx_sched_item_add(&esr->sched_item);
    trace_esr_init(&esr->trace_handle, epl);
    fx_sched_unlock(prev);
    return FX_ESR_OK;
}

//!
//! Destructor of ESR object.
//! User is responsible for calling destructor only for inactive ESR. 
//! Destruction of currently  executing ESR may cause system crash.
//! @param [in,out] esr ESR object to be destroyed.
//! @return FX_ESR_OK if succeeded, error code otherwise.
//! @sa fx_esr_init
//! @sa fx_esr_activate
//!
int 
fx_esr_deinit(fx_esr_t* esr)
{
    fx_sched_state_t prev;
    lang_param_assert(esr != NULL, FX_ESR_INVALID_PTR);
    lang_param_assert(fx_esr_is_valid(esr), FX_ESR_INVALID_OBJ);
    fx_sched_lock(&prev);
    fx_rtp_deinit(&esr->rtp);
    fx_sched_item_remove(&esr->sched_item);
    trace_esr_deinit(&esr->trace_handle, 0);
    fx_sched_unlock(prev);
    return FX_ESR_OK;
}

//!
//! Activation of the ESR object.
//! Tryings to activate already activated ESR are ignored.
//! If activated ESR has greater priority than caller, calling ESR will be
//! interrupted immediately.
//! @param [in,out] esr ESR object to be activated.
//! @param [in] msg Message to be sent to the ESR.
//! @return FX_ESR_OK if succeeded, error code otherwise.
//! @remark SPL <= SCHED_LEVEL
//! @sa fx_esr_init
//! @sa fx_esr_deinit
//!
int 
fx_esr_activate(fx_esr_t* esr, uintptr_t msg)
{
    fx_esr_context_t* const context = fx_esr_get_context();
    fx_sched_state_t prev;    
    lang_param_assert(esr != NULL, FX_ESR_INVALID_PTR);
    lang_param_assert(fx_esr_is_valid(esr), FX_ESR_INVALID_OBJ);

    fx_sched_lock(&prev);

    if (!msg || !hw_cpu_atomic_cas_ptr((void*) &esr->msg, NULL, (void*) msg))
    {
        //
        // If scheduler state changed, dispatch interrupt will be requested.
        //
        (void)(fx_sched_item_resume(&esr->sched_item));
    }

    trace_esr_activate(
        &context->current_esr->trace_handle, 
        esr->trace_handle
    );

    fx_sched_unlock(prev);
    return FX_ESR_OK;
}

//!
//! Stops execution of current ESR. It also causes return to interrupted ESR in 
//! case of nested ESRs.
//! @sa fx_esr_init
//! @sa fx_esr_activate
//! @remark SPL = LOW
//!
void 
fx_esr_exit(void)
{
    fx_esr_context_t* const context = fx_esr_get_context();
    fx_esr_t* const current_esr = context->current_esr;
    hal_intr_frame_t* interrupted_context;
    fx_sched_state_t prev;
    fx_sched_lock(&prev);

    // 
    // Save pointer to previous context to local variable because "current ESR" 
    // will be switched  in this function.
    //
    interrupted_context = current_esr->intr_context;

    //
    // Save interrupted context in order to allocate new interrupt frame just 
    // after interrupted frame, and to prevent stack overflow due to multiple 
    // activations of ESR from itself.
    //
    context->virtual_frame = interrupted_context;
    
    //
    // Update status of the current ESR (next time it should be re-activated by 
    // the scheduler).
    //
    current_esr->active = false;

    //
    // Request rescheduling.
    //
    fx_sched_item_suspend(&current_esr->sched_item);

    //
    // Reset message field, now the ESR may be re-activated by "activate" call.
    //
    current_esr->msg = 0;
    trace_esr_exit(&current_esr->trace_handle, &interrupted_esr->trace_handle); 
    context->current_esr = NULL;
    fx_sched_unlock(prev);
}

//!
//! Raise current EPL level. After EPL raised only ESRs with higher EPL can 
//! preempt current one. It blocks all ESRs with EPL lower or equal to new EPL.
//! @param new_epl EPL to be set. Must be greater than EPL associated with 
//! current ESR.
//! @return Previous EPL.
//! @sa fx_esr_lower_pl
//!
epl_t 
fx_esr_raise_pl(epl_t new_epl)
{
    fx_esr_context_t* const context = fx_esr_get_context();
    fx_esr_t* const me = context->current_esr;
    epl_t ret_value = hw_cpu_atomic_swap(&me->epl, new_epl);
    trace_esr_raise_epl(&me->trace_handle, new_epl);
    return ret_value;
}

//!
//! Lower current EPL level.
//! It unblocks all ESR with EPL greater than new (lowered) EPL value. 
//! Preemption may occur.
//! @param new_epl EPL to be set. Must be lower than EPL associated with 
//! current ESR.
//! @sa fx_esr_raise_pl
//!
void    
fx_esr_lower_pl(epl_t new_epl)
{
    fx_esr_context_t* const context = fx_esr_get_context();
    fx_sched_state_t prev;
    fx_esr_t* me;

    fx_sched_lock(&prev);
    me = context->current_esr;
    (void)hw_cpu_atomic_swap(&me->epl, new_epl);
    fx_sched_mark_resched_needed();
    trace_esr_lower_epl(&me->trace_handle, new_epl);
    fx_sched_unlock(prev);
}

//!
//! This function is called by HAL at exception SPL in case of exception. 
//! Arg points to saved  registers state (which should be analyzed by user to 
//! determine cause of exception).
//!
void    
fx_trap_handler(unsigned int signal, void* arg)
{
    fx_esr_exception_send(signal, arg);
}

//!
//! Common stub for ESRs.
//!
static void 
fx_esr_stub(void* arg)
{
    fx_esr_t* const self = arg;
    fx_sched_state_t state;

    //
    // Cancel wait for correct handling of case when ESR waits for a message 
    // but activated by explicit call of fx_esr_activate.
    //
    fx_sched_lock(&state);
    fx_sync_wait_rollback(&self->waiter);
    fx_sched_unlock(state);

    self->func(self, self->msg, self->arg);

    //
    // Restore interrupted ESR.
    //
    fx_esr_exit();
}

//!
//! Software interrupt handler. This function is called by HAL at SPL=DISPATCH 
//! when software interrupt has been requested.
//!
void 
fx_dispatch_handler(void)
{
    fx_esr_context_t* const context = fx_esr_get_context();
    fx_esr_t* const cur_esr = context->current_esr;
    hal_intr_frame_t* frame = context->virtual_frame;
    fx_sched_item_t* item;
    fx_esr_t* next;
    fx_sched_state_t prev_state;
    
    //
    // Reset virtual frame. It is saved in local variable.
    //
    context->virtual_frame = NULL;
    
    //
    // Handle DPC queue before dispatching ESRs.
    //  
    fx_dpc_handle_queue();
  
    //
    // Lock scheduler after all DPCs are handled.
    //
    fx_sched_lock_from_disp_spl(&prev_state);

    //
    // Schedule next item. It should be NULL if no items present in scheduler.
    //
    item = fx_sched_get_next();

    //
    // This value is being analyzed only when item != NULL.
    //
    next = lang_containing_record(item, fx_esr_t, sched_item);
    
    //
    // Switch context if current ESR is  changed.
    //
    if (item && (!cur_esr || next->epl < cur_esr->epl))
    {
        fx_dbg_assert(fx_esr_is_valid(next));

        //
        // If virtual frame is not set, it means that rescheduiling is not 
        // caused by fx_esr_exit. If it is caused by lowering of EPL, get 
        // interrupt frame from the processor. In case when rescheduling caused 
        // by exiting from the ESR, virtual interrupt frame will be set in 
        // nonzero value.
        //
        if (!frame)
        {
            frame = hal_intr_frame_get();
        }
        
        //
        // Switch current ESR in module context.
        //
        context->current_esr =  next;    

        //
        // Allocate new interrupt frame and preempt current ESR.
        // If next ESR was not active, new frame will be allocated.
        //
        if (!next->active)
        {
            hal_intr_frame_t* const newframe = hal_intr_frame_alloc(frame);

            next->active = true;
            next->intr_context = frame;

            hal_intr_frame_modify(
                newframe, 
                KER_FRAME_ENTRY, 
                (uintptr_t) fx_esr_stub
            );

            hal_intr_frame_modify(newframe, KER_FRAME_ARG0, (uintptr_t) next); 
            frame = newframe;
        }

        hal_intr_frame_set(frame);
        trace_esr_preemption(&cur_esr->trace_handle, &next->trace_handle);
    }
    
    fx_esr_exception_check(&next->traps);
    fx_sched_unlock_from_disp_spl(prev_state);
}

//
// This function required by sync framework and is used for waiters notify.
//
void 
fx_sync_waiter_notify(fx_sync_waiter_t* self) 
{ 
    //
    // Cast abstract waiter to ESR object and resume the item.
    // It is assumed that message which should be passed into the ESR is already 
    // set. Subsequent activations with fx_esr_activate will be skipped due to 
    // non-null message field in the ESR object.
    //
    fx_esr_t* const esr = lang_containing_record(self, fx_esr_t, waiter);
    (void)(fx_sched_item_resume(&esr->sched_item));
}

//!
//! Attach ESR to message port.
//! Test port object for messages, if there are any, the message will be 
//! delivered to this ESR  object. Otherwise the ESR will be attached to the 
//! port and "wait" for a message, if "wait" option is true. If "wait" option is
//! false the function returns synchronously, without attaching to the port.
//! @param object Message port object.
//! @param wait Wait option. Function will not return synchronously if this 
//! option is true.
//! @return Nonzero value if message has been received from the port.
//!
int 
fx_esr_wait_msg(fx_sync_waitable_t* object, bool wait)
{
    fx_esr_context_t* const context = fx_esr_get_context();
    fx_esr_t* const me = context->current_esr;
    bool object_has_msg = false;
    fx_sched_state_t prev;
    fx_sync_wait_block_t temp_wb = FX_SYNC_WAIT_BLOCK_INITIALIZER(
        &me->waiter, 
        object, 
        &me->msg
    );

    fx_sched_lock(&prev);
    
    //
    // Setup waiter. ESR may be attached only to single message port, so, wait 
    // blocks count and expected notifications are always 1.
    // 
    fx_sync_waiter_prepare(&me->waiter, &me->wb, 1, 1);
    memcpy(&me->wb, &temp_wb, sizeof(me->wb));
        
    //
    // Test message port object. 
    // If object had messages at a moment of call, ESR message field will be 
    // filled with new message via wait block attribute (set earlier). If object 
    // had no messages: if "wait" option is true,  current ESR will be attached 
    // to the port and will listen to new messages, otherwise (if "wait" option 
    // is false) ESR stays unattached and its message field remains unchanged.
    //
    object_has_msg = object->test_wait(object, &me->wb, wait);

    //
    // If wait option is true, this function will no return in any way.
    //
    if (wait)
    {
        context->virtual_frame = me->intr_context;
        context->current_esr = NULL;
        me->active = false;
        fx_sched_item_suspend(&me->sched_item);

        //
        // Wait notifications may be received just after call of test_and_wait 
        // function, before the item suspended (so, these notifications will be 
        // skipped, because by spec resuming of already running objects should 
        // be ignored). If waiter is satisfied at this point it means that 
        // notification has been received before this point (maybe, before 
        // suspend) and ESR should be resumed explicitly in order to skip 
        // waiting. Another case if the port had message and wait flag was set 
        // by user, ESR should be restarted with received message 
        // (suspend/resume pair guarantees rescheduling).
        //
        if (object_has_msg || fx_sync_is_waiter_satisfied(&me->waiter))
        {
            fx_sched_item_resume(&me->sched_item);
        }
    }
    fx_sched_unlock(prev);
    return (int) object_has_msg;
}

