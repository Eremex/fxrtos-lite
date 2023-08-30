#ifndef _FXRTOS_STANDARD_RV32I_GNU_HEADER_
#define _FXRTOS_STANDARD_RV32I_GNU_HEADER_

/** 
  ******************************************************************************
  *  @file   standard-rv32i-gnu.h
  *  @brief  Kernel options.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  $$LICENSE$
  *****************************************************************************/

#include FX_INTERFACE(HW_CPU)
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

FX_METADATA(({ interface: [FXRTOS, STANDARD_RV32I_GNU] }))

#endif

