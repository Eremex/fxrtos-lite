#ifndef _HW_CPU_RV32I_HEADER_
#define _HW_CPU_RV32I_HEADER_

/** 
  ******************************************************************************
  *  @file   RV32I/hw_cpu.h
  *  @brief  Low-level utilities and CPU-specific functions and definitions.        
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

#include FX_INTERFACE(LANG_TYPES)

enum
{
    HW_CPU_TIMER_VECT = 7,
    HW_CPU_SWI_VECT = 3,

    HW_CPU_MSTATUS_MIE = 8,
    HW_CPU_MIE_MSIE = 8,
};

//!
//! System CSRs access.
//!
uintptr_t hw_cpu_mscratch_get(void);
void hw_cpu_mscratch_set(uintptr_t);
uintptr_t hw_cpu_mstatus_get(void);
uintptr_t hw_cpu_mie_get(void);
void hw_cpu_mie_set(uintptr_t);
void hw_cpu_msie_set(unsigned int);

//!
//! Memory barrier.
//!
void hw_cpu_dmb(void);

//!
//! Atomic compare-and-swap (CAS).
//! @param [in,out] p Pointer to atomic variable.
//! @param [in] c Value to be compared with target atomic.
//! @param [in] v Value to be written into atomic in case if target value and 
//! comparand are equal.
//! @return Previous value of target. If this value is equal to comparand it 
//! means that target value
//! has been overwritten by newval.
//!
unsigned int hw_cpu_atomic_cas(volatile unsigned* p, unsigned c, unsigned v);

//!
//! Atomic swapping of value in memory.
//! @param [in,out] arg Pointer to atomic variable.
//! @param [in] newval Value to be written into atomic.
//! @return Previous value of target.
//!
unsigned int hw_cpu_atomic_swap(volatile unsigned int* p, unsigned int newval);

//
// For those platforms where size of data is equal to size of pointer, just
// reuse existing data atomics for pointer types.
//
#define hw_cpu_atomic_cas_ptr(p, c, v) \
((void*) hw_cpu_atomic_cas((volatile unsigned*)(p),(unsigned)(c),(unsigned)(v)))

//
// For those platforms where size of data is equal to size of pointer, just 
// reuse existing data atomics for pointer types.
//
#define hw_cpu_atomic_swap_ptr(p, newval) \
    ((void*) hw_cpu_atomic_swap((volatile unsigned*)(p), (unsigned)(newval)))
                    
//!
//! Atomic addition to value in memory.
//! @param [in,out] arg Pointer to atomic variable.
//! @param [in] add Value to be added to atomic.
//! @return Previous value of target.
//!
unsigned int hw_cpu_atomic_add(volatile unsigned int* arg, unsigned int add);

//!
//! Atomic subtraction from value in memory.
//! @param [in,out] arg Pointer to atomic variable.
//! @param [in] sub Value to be subtracted from atomic.
//! @return Previous value of target.
//!
#define hw_cpu_atomic_sub(arg, sub) (hw_cpu_atomic_add((arg), -(sub)))

//!
//! Atomic increment.
//! @param [in,out] arg Pointer to atomic variable.
//! @return Current (incremented) value of target.
//!
#define hw_cpu_atomic_inc(arg) (hw_cpu_atomic_add((arg), 1) + 1)     

//!
//! Atomic decrement.
//! @param [in,out] arg Pointer to atomic variable.
//! @return Current (decremented) value of target.
//!
#define hw_cpu_atomic_dec(arg) (hw_cpu_atomic_add((arg), -1) - 1)

//
// Helper functions for bit counting and BSR/BSF implementation.
//
unsigned int hw_cpu_clz(unsigned int arg);
unsigned int hw_cpu_ctz(unsigned int arg);

//!
//! Places CPU to implementation-specific low-power state until next interrupt. 
//!
void hw_cpu_idle(void);

//!
//! Enable all external interrupts. 
//!
void hw_cpu_intr_enable(void);

//!
//! Disable all external interrupts. 
//!
void hw_cpu_intr_disable(void);

FX_METADATA(({ interface: [HW_CPU, RV32I] }))

#endif
