#ifndef _HAL_INTR_FRAME_ARMv6M_V1_HEADER_
#define _HAL_INTR_FRAME_ARMv6M_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   CortexM/intr_v6m/hal_intr_frame.h
  *  @brief  Interrupt frame management interface.
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

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(HW_CPU)

FX_METADATA(({ interface: [HAL_INTR_FRAME, ARMv6M_V1] }))

//!
//! Interrupt frame. It appears at thread stack on interrupt entry.
//! Since ARMv6M and ARMv7M have compatible interrupt frame this module is used 
//! on both. Note: order of hi_regs depends from architecture. On ARMv7M chips 
//! it appears as linear mapping R4 = hi_regs[0]...R11 = hi_regs[7]. ARMv6M 
//! does not support stm instruction, so, frame is saved as two 4-dword chunks 
//! R8 = hi_regs[0]...R11 = hi_regs[3] and R4 = hi_regs[4]...R7 = hi_regs[7].
//!
typedef struct _hal_intr_frame_t 
{
    uint32_t hi_regs[8];      // +00 <--- software-supplied frame.
    uint32_t r0;              // +32 <--- hardware-supplied frame.
    uint32_t r1;              // +36
    uint32_t r2;              // +40
    uint32_t r3;              // +44
    uint32_t r12;             // +48
    uint32_t lr;              // +52
    uint32_t return_addr;     // +56
    uint32_t xPSR;            // +60
}
hal_intr_frame_t;

enum { KER_FRAME_ENTRY, KER_FRAME_ARG0, USR_FRAME_ENTRY, USR_FRAME_ARG0 };

hal_intr_frame_t* hal_intr_frame_alloc(hal_intr_frame_t* frame);
void hal_intr_frame_modify(hal_intr_frame_t* frame, int reg, uintptr_t val);
hal_intr_frame_t* hal_intr_frame_get(void);
void hal_intr_frame_set(hal_intr_frame_t* frame_ptr);

//!
//! Deferred context switching.
//! Interrupt frame may only be switched in context of dispatch interrupt 
//! handler.
//! @param [in] frame New interrupt frame to be set.
//! @return Previous interrupt frame.
//!
static inline hal_intr_frame_t*
hal_intr_frame_switch(hal_intr_frame_t* frame)
{
    return hw_cpu_set_psp(frame);
}

#endif
