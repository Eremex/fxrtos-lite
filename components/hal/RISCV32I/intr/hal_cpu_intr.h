#ifndef _HAL_CPU_INTR_RV32I_HEADER_
#define _HAL_CPU_INTR_RV32I_HEADER_

/** 
  ******************************************************************************
  *  @file   RISCV32I/intr/hal_cpu_intr.h
  *  @brief  HAL interrupt implementation for RISCV.
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

#include FX_INTERFACE(CFG_OPTIONS)
#include FX_INTERFACE(LANG_TYPES)

//!
//! SPL level constants. All ISRs use single level, interrupts priority are
//! enforced by hardware, ISR level is only used to distinct ISR environment
//! from others.
//!
typedef enum
{
    SPL_SYNC = 0,       //!< SYNC level is above all ISR levels.
    SPL_DISPATCH = 0,   //!< Dispatch interrupt handler.
    SPL_ISR = 1,        //!< Common name for ISR levels.
    SPL_LOW = 0xffff,   //!< Application program level.
}
spl_t;

//!
//! Interrupt frame. It appears on the thread stack as a result of interrupt.
//!
typedef struct _hal_intr_frame_t 
{
    uint32_t pc;
    uint32_t ra;
    uint32_t t[7];
    uint32_t a[8];
    uint32_t s[12];
}
hal_intr_frame_t;

enum { KER_FRAME_ENTRY, KER_FRAME_ARG0 };

extern hal_intr_frame_t* volatile g_hal_intr_stack_frame;

#define hal_intr_frame_get() (g_hal_intr_stack_frame)
#define hal_intr_frame_set(frame) (g_hal_intr_stack_frame) = (frame)

hal_intr_frame_t* hal_intr_frame_alloc(hal_intr_frame_t* frame);
void hal_intr_frame_modify(hal_intr_frame_t* frame, int reg, uintptr_t val);
hal_intr_frame_t* hal_intr_frame_switch(hal_intr_frame_t* new_frame);

#define hal_intr_ctor()
extern unsigned int hal_intr_get_current_vect(void);
extern void fx_tick_handler(void);
extern void fx_intr_handler(void);
extern void fx_dispatch_handler(void);

spl_t hal_async_raise_spl(const spl_t spl);
void hal_async_lower_spl(const spl_t spl);
spl_t hal_async_get_current_spl(void);
void hal_async_request_swi(spl_t spl);

//------------------------------------------------------------------------------

FX_METADATA(({ interface: [HAL_CPU_INTR, RV32I] }))

//------------------------------------------------------------------------------

FX_METADATA(({ options: [                                                                           
    HAL_INTR_STACK_SIZE: {                                                        
        type: int, range: [0x100, 0xffffffff], default: 0x1000,                     
        description: "Size of the interrupt stack (in bytes)."}]}))

#endif
