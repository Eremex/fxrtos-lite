;/** 
; ******************************************************************************
; *  @file   ARMv6-M/hw_cpu.S
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

;FX_METADATA(({ implementation: [HW_CPU, ARMv6M_V1] }))

ASM_ENTRY1(hw_cpu_dmb)
  ASM_ENTRY2(hw_cpu_dmb)
    dmb
    bx lr
  ENDF 

ASM_ENTRY1(hw_cpu_get_ipsr)
  ASM_ENTRY2(hw_cpu_get_ipsr)
    mrs r0, ipsr
    bx lr
  ENDF 

ASM_ENTRY1(hw_cpu_intr_disable)
  ASM_ENTRY2(hw_cpu_intr_disable)
    cpsid i
    bx lr
  ENDF

ASM_ENTRY1(hw_cpu_intr_enable)
  ASM_ENTRY2(hw_cpu_intr_enable)
    cpsie i
    bx lr
  ENDF 

ASM_ENTRY1(hw_cpu_set_psp)
  ASM_ENTRY2(hw_cpu_set_psp)
    mrs r1, psp
    msr psp, r0
    mov r0, r1
    isb
    bx lr
  ENDF 

ASM_ENTRY1(hw_cpu_get_psp)
  ASM_ENTRY2(hw_cpu_get_psp)
    mrs r0, psp
    bx lr
  ENDF 

ASM_ENTRY1(hw_cpu_idle)
  ASM_ENTRY2(hw_cpu_idle)
    bx  lr
  ENDF 

ASM_ENTRY1(hw_cpu_get_primask)
  ASM_ENTRY2(hw_cpu_get_primask)
    mrs r0, primask
    bx lr 
  ENDF 

ASM_ENTRY1(hw_cpu_switch_stack_to_psp)
  ASM_ENTRY2(hw_cpu_switch_stack_to_psp)
    mov r0, sp
    msr psp, r0
    movs r0, #2
    msr control, r0
    isb
    movs r0, #0
    msr msp, r0
    bx lr 
  ENDF

ASM_ENTRY1(hw_cpu_set_msp)
  ASM_ENTRY2(hw_cpu_set_msp)
    msr msp, r0
    bx lr 
  ENDF

;//
;// (r0=ptr, r1=newval)
;//
ASM_ENTRY1(hw_cpu_atomic_swap)
  ASM_ENTRY2(hw_cpu_atomic_swap)
    push {r2-r3}
    dmb
    mrs r2, primask
    cpsid i
    ldr r3, [r0]
    str r1, [r0]
    mov r0, r3
    dmb
    msr primask, r2
    pop {r2-r3}
    bx lr
  ENDF 

;//
;// (r0=ptr, r1=addend)
;//
ASM_ENTRY1(hw_cpu_atomic_add)
  ASM_ENTRY2(hw_cpu_atomic_add)
    push {r2-r3}
    dmb
    mrs r2, primask
    cpsid i
    ldr r3, [r0]
    add r1, r1, r3
    str r1, [r0]
    mov r0, r3
    dmb
    msr primask, r2
    pop {r2-r3}
    bx lr
  ENDF

;//
;// (r0=ptr, r1=comparand, r2=new_value)
;//
ASM_ENTRY1(hw_cpu_atomic_cas)
  ASM_ENTRY2(hw_cpu_atomic_cas)
    push {r3-r4}
    dmb
    mrs r4, primask
    cpsid i
    ldr r3, [r0]
    cmp r3, r1
    bne hw_cpu_atomic_cas_exit
    str r2, [r0]
label(hw_cpu_atomic_cas_exit)
    dmb
    mov r0, r3
    msr primask, r4
    pop {r3-r4}
    bx lr
  ENDF 

ASM_ENTRY1(hw_cpu_clz)
  ASM_ENTRY2(hw_cpu_clz)
    push {r1-r3}
    movs r1, #31
label(hw_cpu_clz_1)
    movs r2, #1
    mov r3, r0
    lsls r2, r1
    ands r3, r2
    bne hw_cpu_clz_exit
    subs r1, #1  
    adds r3, r1, #1
    bne hw_cpu_clz_1
label(hw_cpu_clz_exit)
    adds r1, r1, #1
    movs r0, #32
    subs r0, r0, r1
    pop {r1-r3}
    bx lr
  ENDF 

ASM_ENTRY1(hw_cpu_ctz)
  ASM_ENTRY2(hw_cpu_ctz)
    push {r1-r3}
    movs r1, #0
label(hw_cpu_ctz_1)
    movs r2, #1
    mov r3, r0
    lsls r2, r1
    ands r3, r2
    bne hw_cpu_ctz_exit
    adds r1, #1
    cmp r1, #32
    bne hw_cpu_ctz_1
label(hw_cpu_ctz_exit)
    mov r0, r1
    pop {r1-r3}
    bx lr
  ENDF

    ENDFILE
