/** 
  ******************************************************************************
  *  @file   RISCV32I/intr/hal_cpu_intr0.S
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
#include FX_INTERFACE(LANG_ASM)

;FX_METADATA(({ implementation: [HAL_CPU_INTR, RV32I] }))

#define MSTATUS_MPP_M (3 << 11)
#define MSTATUS_MPIE (1 << 7)
#define CONTEXT_SIZE (29 * 4)

#define SAVE____GPRS  \
    sw  ra,  4(sp);   \
    sw  x5,  8(sp);   \
    sw  x6,  12(sp);  \
    sw  x7,  16(sp);  \
    sw  x28, 20(sp);  \
    sw  x29, 24(sp);  \
    sw  x30, 28(sp);  \
    sw  x31, 32(sp);  \
    sw  x10, 36(sp);  \
    sw  x11, 40(sp);  \
    sw  x12, 44(sp);  \
    sw  x13, 48(sp);  \
    sw  x14, 52(sp);  \
    sw  x15, 56(sp);  \
    sw  x16, 60(sp);  \
    sw  x17, 64(sp);  \
    sw  x8,  68(sp);  \
    sw  x9,  72(sp);  \
    sw  x18, 76(sp);  \
    sw  x19, 80(sp);  \
    sw  x20, 84(sp);  \
    sw  x21, 88(sp);  \
    sw  x22, 92(sp);  \
    sw  x23, 96(sp);  \
    sw  x24, 100(sp); \
    sw  x25, 104(sp); \
    sw  x26, 108(sp); \
    sw  x27, 112(sp);  

#define LOAD____GPRS  \
    lw  ra,  4(sp);   \
    lw  x5,  8(sp);   \
    lw  x6,  12(sp);  \
    lw  x7,  16(sp);  \
    lw  x28, 20(sp);  \
    lw  x29, 24(sp);  \
    lw  x30, 28(sp);  \
    lw  x31, 32(sp);  \
    lw  x10, 36(sp);  \
    lw  x11, 40(sp);  \
    lw  x12, 44(sp);  \
    lw  x13, 48(sp);  \
    lw  x14, 52(sp);  \
    lw  x15, 56(sp);  \
    lw  x16, 60(sp);  \
    lw  x17, 64(sp);  \
    lw  x8,  68(sp);  \
    lw  x9,  72(sp);  \
    lw  x18, 76(sp);  \
    lw  x19, 80(sp);  \
    lw  x20, 84(sp);  \
    lw  x21, 88(sp);  \
    lw  x22, 92(sp);  \
    lw  x23, 96(sp);  \
    lw  x24, 100(sp); \
    lw  x25, 104(sp); \
    lw  x26, 108(sp); \
    lw  x27, 112(sp);  

/*TODO: context save/restore with interrupts enabled. */

ASM_ENTRY1(hal_intr_entry)
    addi    sp, sp, -CONTEXT_SIZE
    SAVE____GPRS
    csrr    t0, mepc
    sw      t0, 0(sp)
    csrr    a0, mcause
    srli    a1, a0, 31
    beqz    a1, synchronous_trap
    la      t0, g_hal_intr_nesting
    lw      t1, 0(t0)
    addi    t2, t1, 1
    sw      t2, 0(t0)
    bnez    t1, asynchronous_intr
    la      t0, g_hal_intr_stack_frame
    sw      sp, 0(t0)
    la      sp, g_hal_intr_stack
    addi    sp, sp, HAL_INTR_STACK_SIZE
asynchronous_intr:
    jal     hal_intr_handler
    la      t0, g_hal_intr_nesting
    lw      t1, 0(t0)
    addi    t1, t1, -1
    sw      t1, 0(t0)
    bnez    t1, context_restore
    la      t0, g_hal_intr_stack_frame
    lw      sp, 0(t0)
context_restore:
    li      t0, (MSTATUS_MPP_M | MSTATUS_MPIE)
    csrw    mstatus, t0
    lw      t0, 0(sp)
    csrw    mepc, t0
    LOAD____GPRS
    addi    sp, sp, CONTEXT_SIZE
    mret
synchronous_trap:
    jal     hal_trap_handler
    j       context_restore


ASM_ENTRY1(hal_intr_check_swi)
    addi    sp, sp, -CONTEXT_SIZE
    SAVE____GPRS
    sw      ra, 0(sp)
    csrc    mstatus, 8
    la      t0, g_hal_intr_nesting
    li      t1, 1
    sw      t1, 0(t0)
    la      t0, g_hal_intr_stack_frame
    sw      sp, 0(t0)
    la      sp, g_hal_intr_stack
    addi    sp, sp, HAL_INTR_STACK_SIZE
    jal     hal_swi_handler
    la      t0, g_hal_intr_nesting
    sw      zero, 0(t0)
    la      t0, g_hal_intr_stack_frame
    lw      sp, 0(t0)
    lw      t0, 0(sp)
    csrw    mepc, t0
    li      t0, (MSTATUS_MPP_M | MSTATUS_MPIE)
    csrw    mstatus, t0
    LOAD____GPRS
    addi    sp, sp, CONTEXT_SIZE
    mret
