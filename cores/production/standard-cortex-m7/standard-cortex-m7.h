#ifndef _FXRTOS_STANDARD_CM7_HEADER_
#define _FXRTOS_STANDARD_CM7_HEADER_

/** 
  ******************************************************************************
  *  @file   standard-cortex-m7.h
  *  @brief  Kernel dependencies.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2022.
  *  $$LICENSE$
  *****************************************************************************/

#include FX_INTERFACE(HAL_INIT)
#include FX_INTERFACE(HAL_CPU_INTR)
#include FX_INTERFACE(FX_TIMER)
#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(FX_DPC)
#include FX_INTERFACE(FX_SEM)
#include FX_INTERFACE(FX_MUTEX)
#include FX_INTERFACE(FX_MSGQ)
#include FX_INTERFACE(FX_BLOCK_POOL)
#include FX_INTERFACE(FX_EV_FLAGS)
#include FX_INTERFACE(FX_RWLOCK)
#include FX_INTERFACE(FX_COND)
#include FX_INTERFACE(FX_MEM_POOL)

FX_METADATA(({ interface: [FXRTOS, STANDARD_CORTEX_M7] }))

#endif
