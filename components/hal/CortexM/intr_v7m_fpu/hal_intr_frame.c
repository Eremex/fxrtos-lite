/** 
  ******************************************************************************
  *  @file   CortexM/intr_v7m_fpu/hal_intr_frame.c
  *  @brief  HAL interrupt implementation for CortexM4F.
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

#include FX_INTERFACE(HAL_INTR_FRAME)

FX_METADATA(({ implementation: [HAL_INTR_FRAME, ARMv7M_FPU] }))

//!
//! Interrupt frame modifying.
//! @param [in] frame Pointer to interrupt frame.
//! @param [in] reg Register to be modified (at least KER_FRAME_ENTRY and 
//! KER_FRAME_ARG0 should be supported).
//! @param [in] val Register value.
//! @warning This function may be called ONLY in context of interrupt handlers.
//!
void 
hal_intr_frame_modify(hal_intr_frame_t* frame, int reg, uintptr_t val)
{
    switch (reg)
    {
    case KER_FRAME_ENTRY: frame->u.gp.regs_hw.return_addr = val; break;
    case KER_FRAME_ARG0: frame->u.gp.regs_hw.r0 = val; break;
    default: break;
    }
}

//!
//! Allocates and initializaes new interrupt frame, relative to given base.
//! @param [in] base Pointer to allocation base.
//! @warning This function may be called ONLY in context of interrupt handlers.
//!
hal_intr_frame_t* 
hal_intr_frame_alloc(hal_intr_frame_t* base)
{
    //
    // ARM7M does not support ARM instruction set, T-bit must be always set.
    //
    enum
    {
        XPSR_T_BIT = (1 << 24)  
    };

    hal_intr_frame_t* frame = (hal_intr_frame_t*) (
        (intptr_t)base - sizeof(hal_nonfp_intr_frame_t)
    );

    frame->u.gp.regs_hw.r0 = 0;
    frame->u.gp.regs_hw.r1 = 0;
    frame->u.gp.regs_hw.r2 = 0;
    frame->u.gp.regs_hw.r3 = 0;
    frame->u.gp.regs_hw.r12 = 0;
    frame->u.gp.regs_hw.lr = 0;
    frame->u.gp.regs_hw.return_addr = 0;
    frame->u.gp.regs_hw.xPSR = XPSR_T_BIT;

    frame->u.gp.regs_sw.r4 = 0;
    frame->u.gp.regs_sw.r5 = 0;
    frame->u.gp.regs_sw.r6 = 0;
    frame->u.gp.regs_sw.r7 = 0;
    frame->u.gp.regs_sw.r8 = 0;
    frame->u.gp.regs_sw.r9 = 0;
    frame->u.gp.regs_sw.r10 = 0;
    frame->u.gp.regs_sw.r11 = 0;
    frame->u.gp.regs_sw.exc_return_code = 0xfffffffd;
    return frame;
}

//!
//! Returns interrupted context.
//! @return Pointer to interrupt frame.
//! @warning This function may be called ONLY in context of interrupt handlers.
//!
hal_intr_frame_t*
hal_intr_frame_get(void)
{
    return hw_cpu_get_psp();
}

//!
//! Sets current interrupt frame.
//! @param [in] frame_ptr Pointer to interrupt frame (interrupted context).
//! @warning This function may be called ONLY in context of interrupt handlers.
//!
void 
hal_intr_frame_set(hal_intr_frame_t* frame_ptr)
{
    (void) hw_cpu_set_psp(frame_ptr);
}
