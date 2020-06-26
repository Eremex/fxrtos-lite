#ifndef _HW_MPU_CM3_HEADER_
#define _HW_MPU_CM3_HEADER_

/** 
  ******************************************************************************
  *  @file   hw_mpu.h
  *  @brief  MPU helper functions.
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
    HW_MPU_RGN_PERM_PRV_NO_USR_NO  = 0x00, // no access in any mode
    HW_MPU_RGN_PERM_PRV_RW_USR_NO  = 0x01, // privileged RW, user no access
    HW_MPU_RGN_PERM_PRV_RW_USR_RO  = 0x02, // privileged RW, user RO
    HW_MPU_RGN_PERM_PRV_RW_USR_RW  = 0x03, // RW in any mode
    HW_MPU_RGN_PERM_PRV_RO_USR_NO  = 0x05, // privileged RO, user no access
    HW_MPU_RGN_PERM_PRV_RO_USR_RO  = 0x06  // RO in any mode
};

void hw_mpu_set_enable(bool enabled, bool en_priv, bool en_fault);
unsigned int hw_mpu_region_get_count(void);
void hw_mpu_region_set_enable(unsigned int region, bool enabled);

void hw_mpu_region_set_access(
    unsigned int region, 
    uintptr_t addr, 
    size_t sz, 
    unsigned int attr
);

bool hw_mpu_region_get(
    unsigned int region, 
    uintptr_t* addr, 
    size_t* sz, 
    unsigned int* attr
);

FX_METADATA(({ interface: [HW_MPU, CM3] }))

#endif
