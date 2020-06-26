#ifndef _HW_CPU_ARMv6M_V1_HEADER_
#define _HW_CPU_ARMv6M_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   ARMv6-M/hw_cpu.h
  *  @brief  Low-level utilities and CPU-specific functions and definitions.        
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

//
// CPU-specific instructions and registers.
//
void hw_cpu_dmb(void);
unsigned int hw_cpu_get_ipsr(void);
unsigned int hw_cpu_get_primask(void);
void* hw_cpu_get_psp(void);
void* hw_cpu_set_psp(void* new_psp);
unsigned int hw_cpu_get_control(void);
void hw_cpu_set_control(unsigned int control);
void hw_cpu_set_msp(void*);
void hw_cpu_switch_stack_to_psp(void);

//!
//! Atomic compare-and-swap (CAS).
//! @param [in,out] target Pointer to atomic variable.
//! @param [in] comparand Value to be compared with target atomic.
//! @param [in] newval Value to be written into atomic in case if target value 
//! and comparand are equal.
//! @return Previous value of target. If this value is equal to comparand it 
//! means that target value has been overwritten by newval.
//!
unsigned int hw_cpu_atomic_cas(
    volatile unsigned int* target, 
    unsigned int comparand, 
    unsigned int newval
);

//!
//! Atomic swapping of value in memory.
//! @param [in,out] p Pointer to atomic variable.
//! @param [in] newval Value to be written into atomic.
//! @return Previous value of target.
//!
unsigned int hw_cpu_atomic_swap(volatile unsigned int* p, unsigned int newval);

//
// For those platforms where size of data is equal to size of pointer, just 
// reuse existing data atomics for pointer types.
//
#define hw_cpu_atomic_cas_ptr(p, c, v) \
    ((void*)hw_cpu_atomic_cas((volatile unsigned*)(p),(unsigned)(c),(unsigned)(v)))

//
// For those platforms where size of data is equal to size of pointer, just 
// reuse existing data atomics for pointer types.
//
#define hw_cpu_atomic_swap_ptr(p, v) \
    ((void*) hw_cpu_atomic_swap((volatile unsigned int*)(p), (unsigned int)(v)))
                    
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
// Helper functions for bit counting.
//
unsigned int hw_cpu_clz(unsigned int arg);
unsigned int hw_cpu_ctz(unsigned int arg);

//!
//! Place processor to implementation-specific low-power state until next 
//! interrupt occurs. 
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

FX_METADATA(({ interface: [HW_CPU, ARMv6M_V1] }))

#endif
