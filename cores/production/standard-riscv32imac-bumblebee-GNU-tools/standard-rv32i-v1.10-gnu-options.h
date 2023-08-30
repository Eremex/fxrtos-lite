#ifndef _CFG_OPTIONS_RV32I_GNU_HEADER_
#define _CFG_OPTIONS_RV32I_GNU_HEADER_

/** 
  ******************************************************************************
  *  @file   standard-rv32i-v1.10-gnu-options.h
  *  @brief  Kernel options.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  $$LICENSE$
  *****************************************************************************/

#define FX_SCHED_ALG_PRIO_NUM 32
#define FX_TIMER_THREAD_PRIO 1                              
#define FX_TIMER_THREAD_STACK_SIZE 0x400
#define HAL_INTR_TIMER_MCAUSE 0x80000007
#define HAL_INTR_MCAUSE_EXCCODE_MASK 0x80000FFF
#define HAL_INTR_STACK_SIZE 0x400
#define RTL_MEM_POOL_MAX_CHUNK 15

#define RV_SPEC_MSTATUS_MPP_M (3 << 11)
#define RV_SPEC_MSTATUS_MPIE (1 << 7)
#define RV_SPEC_MSTATUS_MIE 8
#define RV_SPEC_INT_RET	mret

FX_METADATA(({ interface: [CFG_OPTIONS, STANDARD_RV32I_GNU] }))

#endif
