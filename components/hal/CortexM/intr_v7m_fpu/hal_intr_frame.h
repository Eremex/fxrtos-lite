#ifndef _HAL_INTR_FRAME_ARMv7M_FPU_HEADER_
#define _HAL_INTR_FRAME_ARMv7M_FPU_HEADER_

/** 
  ******************************************************************************
  *  @file   CortexM/intr_v7m_fpu/hal_cpu_intr.h
  *  @brief  HAL interrupt implementation for CortexM with FPU support.
  *  Note: Before FPU using it must be enabled first:
  *  FPU->FPCCR |= (0x3UL << 30UL);
  *  CPACR (0xE000ED88) |= (0xf << 20);
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

//!
//! Software-supplied part of interrupt frame. 
//! It is saved for both FPU-threads and nonFPU and is used as an type indicator
//! for rest of the context.
//!
typedef struct
{
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t exc_return_code;
}
hal_sw_intr_frame_t;

//!
//! Hardware-supplied interrupt frame for integer context. It is defined by the
//! architecture.
//!
typedef struct 
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t return_addr;
    uint32_t xPSR;
}
hal_hw_intr_frame_t;

//!
//! Full interrupt frame with FPU context.
//!
typedef struct 
{
    hal_sw_intr_frame_t regs_sw;
    uint32_t fp_sw[16];
    hal_hw_intr_frame_t regs_hw;
    uint32_t fp_hw[16];
    uint32_t fpscr;    
}
hal_fp_intr_frame_t;

//!
//! Full interrupt frame for threads using integer registers only.
//! 
typedef struct 
{
    hal_sw_intr_frame_t regs_sw;
    hal_hw_intr_frame_t regs_hw; 
}
hal_nonfp_intr_frame_t;

//!
//! Full interrupt frame is the union of two cases: FP and nonFP frames.
//!
typedef union
{
    hal_nonfp_intr_frame_t gp;
    hal_fp_intr_frame_t fp;
}
hal_intr_frame_u;

//!
//! Context switch code uses interrupt frame by pointer with forward declaration
//! of the structure. To avoid compiler warnings caused by incompatible types 
//! interrupt frame union is embedded into structure with single member, so, 
//! fwd declaration of struct _hal_intr_frame_t is still valid one.
//!
typedef struct _hal_intr_frame_t
{
    hal_intr_frame_u u;
}
hal_intr_frame_t;

//
// Ensure that integer software frame if the first member for both context 
// descriptors. Also ensure that interrupt frame is properly packed.
// For integer frame it must be exactly 17 words:
// (15 integer registers + xPSR + EXC_RETURN code).
// For FPU frame it must be 50 words: integer frame + 32 FP regs + FPSCR.
//
lang_static_assert(offsetof(hal_fp_intr_frame_t, regs_sw) == 0);
lang_static_assert(offsetof(hal_nonfp_intr_frame_t, regs_sw) == 0);
lang_static_assert(sizeof(hal_nonfp_intr_frame_t) == sizeof(uint32_t) * 17);
lang_static_assert(sizeof(hal_fp_intr_frame_t) == sizeof(uint32_t) * 50);

enum { KER_FRAME_ENTRY, KER_FRAME_ARG0 };

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

FX_METADATA(({ interface: [HAL_INTR_FRAME, ARMv7M_FPU] }))

#endif
