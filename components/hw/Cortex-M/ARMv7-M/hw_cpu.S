;/** 
; ******************************************************************************
; *  @file   ARMv7-M/hw_cpu.S
; *  @brief  Low-level utilities and CPU-specific functions.
; ******************************************************************************
; *  Copyright (C) JSC EREMEX, 2008-2020.
; *  Redistribution and use in source and binary forms, with or without 
; *  modification, are permitted provided that the following conditions are met:
; *  1. Redistributions of source code must retain the above copyright notice,
; *     this list of conditions and the following disclaimer.
; *  2. Redistributions in binary form must reproduce the above copyright 
; *     notice, this list of conditions and the following disclaimer in the 
; *     documentation and/or other materials provided with the distribution.
; *  3. Neither the name of the copyright holder nor the names of its 
; *     contributors may be used to endorse or promote products derived from 
; *     this software without specific prior written permission.
; *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
; *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
; *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
; *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
; *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
; *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
; *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
; *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
; *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; *  POSSIBILITY OF SUCH DAMAGE.
; *****************************************************************************/

;//
;// Keil assembler does not allow functional macros in command line.
;// IAR assembler can not use function-like macros in includes.
;// So, in case when fx_interface macro is not defined, it always mean building 
;// project from some IDE, just use appropriate filename.
;//
#ifndef FX_INTERFACE
#include <LANG_ASM.h>
#else
#include FX_INTERFACE(LANG_ASM)
#endif

;FX_METADATA(({ implementation: [HW_CPU, ARMv7M_V1] }))

ASM_ENTRY1(hw_cpu_dmb)
  ASM_ENTRY2(hw_cpu_dmb)
    dmb
    bx    lr
  ENDF 
  
ASM_ENTRY1(hw_cpu_get_ipsr)
  ASM_ENTRY2(hw_cpu_get_ipsr)
    mrs   r0, ipsr
    bx    lr
  ENDF
  
ASM_ENTRY1(hw_cpu_intr_disable)
  ASM_ENTRY2(hw_cpu_intr_disable)
    cpsid i
    bx    lr
  ENDF

ASM_ENTRY1(hw_cpu_get_control)
  ASM_ENTRY2(hw_cpu_get_control)
    mrs   r0, control
    bx    lr
  ENDF

ASM_ENTRY1(hw_cpu_set_control)
  ASM_ENTRY2(hw_cpu_set_control)
    msr   control, r0
    isb
    bx    lr
  ENDF 

ASM_ENTRY1(hw_cpu_intr_enable)
  ASM_ENTRY2(hw_cpu_intr_enable)
    cpsie i
    bx    lr
  ENDF

ASM_ENTRY1(hw_cpu_get_basepri)
  ASM_ENTRY2(hw_cpu_get_basepri)
    mrs   r0, basepri
    bx    lr
  ENDF

ASM_ENTRY1(hw_cpu_set_basepri)
  ASM_ENTRY2(hw_cpu_set_basepri)
    msr   basepri, r0
    bx    lr
  ENDF

ASM_ENTRY1(hw_cpu_set_psp)
  ASM_ENTRY2(hw_cpu_set_psp)
    mrs   r1, psp
    msr   psp, r0
    mov   r0, r1
    isb
    bx    lr
  ENDF

ASM_ENTRY1(hw_cpu_get_psp)
  ASM_ENTRY2(hw_cpu_get_psp)
    mrs   r0, psp
    bx    lr
  ENDF

ASM_ENTRY1(hw_cpu_switch_stack_to_psp)
  ASM_ENTRY2(hw_cpu_switch_stack_to_psp)
    mov r0, sp
    msr psp, r0
    mov r0, #2
    msr control, r0
    isb
    mov r0, #0
    msr msp, r0
    bx lr  
  ENDF

ASM_ENTRY1(hw_cpu_set_msp)
  ASM_ENTRY2(hw_cpu_set_msp)
    msr msp, r0
    bx lr 
  ENDF

ASM_ENTRY1(hw_cpu_idle)
  ASM_ENTRY2(hw_cpu_idle)
    wfi
    bx    lr
  ENDF

ASM_ENTRY1(hw_cpu_clz)
  ASM_ENTRY2(hw_cpu_clz)
    clz   r0, r0
    bx    lr
  ENDF

ASM_ENTRY1(hw_cpu_ctz)
  ASM_ENTRY2(hw_cpu_ctz)
    rbit  r0, r0
    clz   r0, r0
    bx    lr
  ENDF

;//
;// (r0=ptr, r1=newval)
;//
ASM_ENTRY1(hw_cpu_atomic_swap)
  ASM_ENTRY2(hw_cpu_atomic_swap)
    push  {r1-r3}             ;// Save scratch registers.
    dmb                       ;// Complete all previous memory accesses.
label(hw_cpu_atomic_swap_1)
    ldrex r2, [r0]            ;// Load and start monitoring. 
    strex r3, r1, [r0]        ;// Try to store new value in memory.
    cmp   r3, #1            
    beq   hw_cpu_atomic_swap_1;// If store failed goto next cycle.
    clrex                     ;// else stop monitoring and return.
    dmb                       ;// Flush write buffers.
    mov   r0, r2
    pop   {r1-r3}
    bx    lr
  ENDF

;//
;// (r0=ptr, r1=addend)
;//
ASM_ENTRY1(hw_cpu_atomic_add)
  ASM_ENTRY2(hw_cpu_atomic_add)
    push  {r1-r4}             ;// Save scratch registers.
    dmb                       ;// Complete all previous memory accesses.
label(hw_cpu_atomic_add_1)
    ldrex r2, [r0]            ;// Load and start monitoring.
    add   r4, r2, r1          ;// Add addend to the read value.
    strex r3, r4, [r0]        ;// Try to store result.
    cmp   r3, #1 
    beq   hw_cpu_atomic_add_1 ;// If store failed - goto next cycle.
    clrex                     ;// Stop monitoring.
    dmb                       ;// Flush write buffers.
    mov   r0, r2              ;// Set return value.
    pop   {r1-r4}             ;// Cleanup.
    bx    lr                  ;// Return.
  ENDF

;//
;// (r0=ptr, r1=comparand, r2=new_value)
;//
ASM_ENTRY1(hw_cpu_atomic_cas)
  ASM_ENTRY2(hw_cpu_atomic_cas)
    push  {r1-r4}             ;// Save scratch registers.
    dmb                       ;// Complete all previous memory accesses.
label(hw_cpu_atomic_cas_1)
    ldrex r3, [r0]            ;// Get current value  into r3 and start monitor.        
    cmp   r3, r1              ;// Compare read value and comparand.  
    ittt  ne                  ;// if r3 != r1
    clrexne                   ;// then: stop monitoring.
    movne r0, r3              ;// then: set read value as return value.
    bne   hw_cpu_atomic_cas_2 ;// then: goto return.
    strex r4, r2, [r0]        ;// Try to store new value from r2.   
    teq   r4, #0              ;// Test for success.
    itt   eq                  ;// if store succeeded
    moveq r0, r3              ;// then: set read value from r3 as return value
    beq   hw_cpu_atomic_cas_2 ;// then: goto return
    b     hw_cpu_atomic_cas_1 ;// else goto next cycle and try again.
label(hw_cpu_atomic_cas_2)
    dmb                       ;// Flush write buffers.
    pop   {r1-r4}             ;// Cleanup and return.
    bx    lr
  ENDF
    
    ENDFILE
