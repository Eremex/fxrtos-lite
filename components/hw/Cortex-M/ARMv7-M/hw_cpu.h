#ifndef _HW_CPU_ARMv7M_V1_HEADER_
#define _HW_CPU_ARMv7M_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   ARMv7-M/hw_cpu.h
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

//!
//! System control block structure.
//!
typedef volatile struct
{
    uint32_t CPUID;    //!< CPU ID Base Register.                                     
    uint32_t ICSR;     //!< Interrupt Control State Register.                         
    uint32_t VTOR;     //!< Vector Table Offset Register.                             
    uint32_t AIRCR;    //!< Application Interrupt / Reset Control Register.           
    uint32_t SCR;      //!< System Control Register.                                  
    uint32_t CCR;      //!< Configuration Control Register.                           
    uint8_t  SHP[12];  //!< System Handlers Priority Registers.    
    uint32_t SHCSR;    //!< System Handler Control and State Register.                
    uint32_t CFSR;     //!< Configurable Fault Status Register.                       
    uint32_t HFSR;     //!< Hard Fault Status Register.                               
    uint32_t DFSR;     //!< Debug Fault Status Register.                              
    uint32_t MMFAR;    //!< Mem Manage Address Register.                              
    uint32_t BFAR;     //!< Bus Fault Address Register.                               
    uint32_t AFSR;     //!< Auxiliary Fault Status Register.                          
    uint32_t PFR[2];   //!< Processor Feature Register.                               
    uint32_t DFR;      //!< Debug Feature Register.                                   
    uint32_t ADR;      //!< Auxiliary Feature Register.                               
    uint32_t MMFR[4];  //!< Memory Model Feature Register.                            
    uint32_t ISAR[5];  //!< ISA Feature Register.                                     
} 
hw_scb_t;

#define HW_CPU_SCS_BASE (0xE000E000)
#define HW_CPU_SCB_BASE (HW_CPU_SCS_BASE +  0x0D00)
#define HW_SYSTEM_CTL   ((hw_scb_t*) HW_CPU_SCB_BASE)
#define hw_cpu_request_pendsv() HW_SYSTEM_CTL->ICSR = 0x10000000

//
// CPU-specific instructions and registers reads/writes.
//
void hw_cpu_sev(void);
void hw_cpu_dmb(void);
unsigned int hw_cpu_get_ipsr(void);
void* hw_cpu_get_psp(void);
void* hw_cpu_set_psp(void* new_psp);
unsigned int hw_cpu_get_basepri(void);
void hw_cpu_set_basepri(unsigned int);
unsigned int hw_cpu_get_control(void);
void hw_cpu_set_control(unsigned int control);
void hw_cpu_set_msp(void*);
void hw_cpu_switch_stack_to_psp(void);

//!
//! Atomic compare-and-swap (CAS).
//! @param [in,out] p Pointer to atomic variable.
//! @param [in] comparand Value to be compared with target atomic.
//! @param [in] newval Value to be written into atomic in case if target value 
//! and comparand are equal.
//! @return Previous value of target. If this value is equal to comparand it 
//! means that target value has been overwritten by newval.
//!
unsigned int hw_cpu_atomic_cas(
    volatile unsigned int* p, 
    unsigned int comparand, 
    unsigned int newval
);

//!
//! Atomic swapping of value in memory.
//! @param [in,out] p Pointer to atomic variable.
//! @param [in] val Value to be written into atomic.
//! @return Previous value of target.
//!
unsigned int hw_cpu_atomic_swap(volatile unsigned int* p, unsigned int val);

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
// Helper functions for bit counting and BSR/BSF implementation.
//
unsigned int hw_cpu_clz(unsigned int arg);
unsigned int hw_cpu_ctz(unsigned int arg);

//!
//! Places processor to implementation-specific low-power state until next 
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

FX_METADATA(({ interface: [HW_CPU, ARMv7M_V1] }))

#endif
