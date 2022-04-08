#ifndef _CFG_OPTIONS_STD_CM7_HEADER_
#define _CFG_OPTIONS_STD_CM7_HEADER_

/** 
  ******************************************************************************
  *  @file   standard-cortex-m7-options.h
  *  @brief  Kernel options.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2022.
  *  $$LICENSE$
  *****************************************************************************/

#define FX_SCHED_ALG_PRIO_NUM 32
#define FX_TIMER_THREAD_PRIO 1                              
#define FX_TIMER_THREAD_STACK_SIZE 0x400
#define HAL_INIT_INTR_STACK_SIZE 0x400
#define RTL_MEM_POOL_MAX_CHUNK 15
#define HW_PIT_TICK_PERIOD 100000

#ifndef __IAR_SYSTEMS_ASM__
FX_METADATA(({ interface: [CFG_OPTIONS, STANDARD_CORTEX_M7] }))
#endif

#endif
