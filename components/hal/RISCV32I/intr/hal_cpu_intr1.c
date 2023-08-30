/** 
  ******************************************************************************
  *  @file   RISCV32I/intr/hal_cpu_intr1.c
  *  @brief  HAL interrupt implementation for RISCV.
  *
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2020-2023.
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

#include FX_INTERFACE(HAL_CPU_INTR)
#include FX_INTERFACE(HW_CPU)

FX_METADATA(({ implementation: [HAL_CPU_INTR, RV32I] }))

hal_intr_frame_t* volatile g_hal_intr_stack_frame = NULL;
volatile unsigned int g_hal_intr_nesting = 0;
unsigned int g_hal_intr_stack[HAL_INTR_STACK_SIZE / sizeof(unsigned int)];
static volatile spl_t g_hal_intr_current_spl = SPL_SYNC;
static volatile int g_hal_intr_dispatch_req = 0;

extern void hal_intr_check_swi(void);
extern void hal_timer_pre_tick(void);
extern void hal_timer_post_tick(void);

static inline spl_t
_hal_async_spl_set(const spl_t spl)
{
    const spl_t old_spl = g_hal_intr_current_spl;
    g_hal_intr_current_spl = spl;
    return old_spl;
}

//!
//! Calls Os' dispatch interrupt handler.
//! @warning Caller must raise SPL to SPL_ISR.
//!
static inline void
_hal_intr_swi_dispatch(void)
{
    while (g_hal_intr_dispatch_req != 0)
    {
        g_hal_intr_dispatch_req = 0;
        hw_cpu_intr_enable();
        fx_dispatch_handler();
        hw_cpu_intr_disable();
    }
}

//!
//! Sets MIE to value corresponding to new SPL.
//!
spl_t
hal_async_raise_spl(const spl_t spl)
{
    hw_cpu_intr_disable();
    const spl_t old_spl = _hal_async_spl_set(spl);
    return old_spl;
}

//!
//! Lower SPL and set MIE to appropriate value.
//! Triggers dispatch interrupt is it is pending and being unmasked.
//!
void
hal_async_lower_spl(const spl_t spl)
{
    hw_cpu_intr_disable();
    (void)_hal_async_spl_set(spl);

    if (spl == SPL_LOW && g_hal_intr_dispatch_req != 0)
    {
        hal_intr_check_swi();
    }
    else if (spl != SPL_SYNC)
    {
        hw_cpu_intr_enable();
    }
}

//!
//! Get current SPL.
//! @return Current SPL.
//!
spl_t
hal_async_get_current_spl(void)
{
    return g_hal_intr_current_spl;
}

//!
//! Dispatch interrupt request. May be called only at levels SPL_DISPATCH or
//! above. If dispatch is pending then dispatch interrupt handler will be 
//! called on SPL lowering.
//!
void
hal_async_request_swi(spl_t spl)
{
    g_hal_intr_dispatch_req = 1;
    hw_cpu_dmb();
}

//!
//! Called from asm code at interrupt stack as hardware interrupt handler.
//! It is always asynchronous and called with interrupts disabled.
//! Pending dispatch requests will be handled on return to thread from all
//! nesting ISRs.
//!
void
hal_intr_handler(uint32_t mcause)
{
    const spl_t prev_spl = _hal_async_spl_set(SPL_ISR);

    if ((mcause & HAL_INTR_MCAUSE_EXCCODE_MASK) == HAL_INTR_TIMER_MCAUSE)
    {
        hal_timer_pre_tick();
        hw_cpu_intr_enable();
        fx_tick_handler();
        hw_cpu_intr_disable();
        hal_timer_post_tick();
    }
    else
    {
        fx_intr_handler();
    }

    hw_cpu_intr_disable();

    if (prev_spl == SPL_LOW)
    {
        _hal_intr_swi_dispatch();
    }

    _hal_async_spl_set(prev_spl);
}

//!
//! Called from asm code at interrupt stack as software interrupt handler.
//! It is always synchronous and called with interrupts disabled.
//! Context will be resored from actual frame pointer on return from this
//! function.
//! @note Since dispatch software interrupt may be triggered only from threads
//! interrupt nesting is always 1 here, and previous SPL is always SPL_LOW.
//!
void
hal_swi_handler(void)
{
    _hal_async_spl_set(SPL_ISR);
    _hal_intr_swi_dispatch();
    _hal_async_spl_set(SPL_LOW);
}

//!
//! Replace pointer to current interrupt frame.
//! Since SWI handler is not reentrant, this function may not be atomic.
//! @param new_frame New frame value to be set.
//! @return Pointer to previous frame.
//! @warning May be used only in context of dispatch interrupt handler.
//!
hal_intr_frame_t*
hal_intr_frame_switch(hal_intr_frame_t* new_frame)
{
    hal_intr_frame_t* current_frame = hal_intr_frame_get();
    hal_intr_frame_set(new_frame);
    return current_frame;
}

//!
//! Modifying interrupt frame.
//! @param [in] frame Pointer to allocated interrupt frame.
//! @param [in] reg Selection of register to modify 
//! (at least KER_FRAME_(ENTRY|ARG0) should be supported).
//! @param [in] val Register value to be set.
//!
void
hal_intr_frame_modify(hal_intr_frame_t* frame, int reg, uintptr_t val)
{
    switch (reg)
    {
    case KER_FRAME_ENTRY: frame->pc = val; break;
    case KER_FRAME_ARG0: frame->a[0] = val; break;
    default: break;
    }
}

//!
//! Allocates and initializaes new interrupt frame, relative to given base.
//! @param [in] base Pointer to allocation base.
//! @return Pointer to allocated frame.
//!
hal_intr_frame_t*
hal_intr_frame_alloc(hal_intr_frame_t* base)
{
    hal_intr_frame_t* frame = base - 1;
    unsigned int i = 0;

    frame->pc = 0;
    frame->ra = 0;

    for (i = 0; i < sizeof(frame->t) / sizeof(frame->t[0]); ++i)
    {
        frame->t[i] = 0;
    }

    for (i = 0; i < sizeof(frame->a) / sizeof(frame->a[0]); ++i)
    {
        frame->a[i] = 0;
    }

    for (i = 0; i < sizeof(frame->s) / sizeof(frame->s[0]); ++i)
    {
        frame->s[i] = 0;
    }

    return frame;
}
