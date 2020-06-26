/** 
  ******************************************************************************
  *  @file   hw_mpu.c
  *  @brief  MPU helpers for Cortex M3.
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

#include FX_INTERFACE(HW_MPU)
#include FX_INTERFACE(HW_CPU)

FX_METADATA(({ implementation: [HW_MPU, CM3] }))

enum
{
    HW_MPU_ENABLE = 0,
    HW_MPU_ENABLED_ON_FAULT = 1,
    HW_MPU_SHAROW_RGN_ENABLED = 2,
    HW_MPU_RGN_SIZE = 1,
    HW_MPU_RGN_ATTRIB = 24,
};

//!
//! Memory mapped registers to control the MPU.
//!
typedef volatile struct
{
    uint32_t type;
    uint32_t control;
    uint32_t region_number;
    uint32_t region_base;
    uint32_t region_attr;
}  
hw_mpu_t;

#define HW_MPU_BASE  (HW_CPU_SCS_BASE +  0x0D90)  // MPU Base Address.                    
#define HW_MPU       ((hw_mpu_t*) HW_MPU_BASE)    // MPU configuration struct.

//!
//! Controls whole MPU status.
//! @param [in] enabled Controls MPU enabled bit.
//! @param [in] en_priv MPU shadow region is enabled for privileged software.
//! @param [in] en_fault MPU is enabled for HardFault and NMI handlers.
//! @warning Additional memory barriers is required.
//!
void 
hw_mpu_set_enable(bool enabled, bool en_priv, bool en_fault)
{
    const uint32_t control = (
        (en_priv << HW_MPU_SHAROW_RGN_ENABLED) | 
        (en_fault << HW_MPU_ENABLED_ON_FAULT) | 
        (enabled << HW_MPU_ENABLE)
    );

    HW_MPU->control = control;
}

//!
//! Returns number of available MPU regions.
//! @return Number of available regions.
//!
unsigned int 
hw_mpu_region_get_count(void)
{
    return (HW_MPU->type >> 8) & 0xFF;
}

//!
//! Sets region enabled status.
//! @param [in] region Region number. Must be less than value obtained by 
//! @ref hw_mpu_region_get_count.
//! @param [in] enabled Region status to be set.
//! @warning Writing to MPU registers is not atomic, this function should not be
//! preempted by any activity that accesses MPU.
//! @warning This function controls the region enable bit only. Trying to enable
//! region which was not properly initialized with @ref hw_mpu_region_set_access
//! may result in undefined behavior.
//!
void 
hw_mpu_region_set_enable(unsigned int region, bool enabled)
{
    uint32_t region_attr = HW_MPU->region_attr;
    region_attr &= ~(1 << HW_MPU_ENABLE);
    region_attr |= enabled;
    HW_MPU->region_number = region & 0xFF;
    HW_MPU->region_attr = region_attr;
}

//!
//! Sets region attributes.
//! @param [in] region Region number. Must be less than value obtained by 
//! @ref hw_mpu_region_get_count.
//! @param [in] addr Region base address. Must be aligned to size.
//! @param [in] sz Region size expressed as power of two minus 1 
//! (i.e. 64K = 2^16 -> sz = 15).
//! @param [in] attr Region access attributes.
//! @warning Writing to MPU registers is not atomic, this function should not be
//! preempted by any activity that accesses MPU.
//! @warning Region is enabled after the parameters set.
//!
void 
hw_mpu_region_set_access(
    unsigned int region, 
    uintptr_t addr, 
    size_t sz, 
    unsigned int attr)
{
    const uint32_t region_attr = (
        (attr << HW_MPU_RGN_ATTRIB) | 
        (sz << HW_MPU_RGN_SIZE) | 
        (1 << HW_MPU_ENABLE)
    );

    HW_MPU->region_number = region & 0xFF;
    HW_MPU->region_base = addr;
    HW_MPU->region_attr = region_attr;
}

//!
//! Get region desription by its index.
//! @param [in] region Region number. Must be less than value obtained by 
//! @ref hw_mpu_region_get_count.
//! @param [in] addr Region base address.
//! @param [in] sz Region size in bytes.
//! @param [in] attr Region access attributes.
//! @return True if region is enabled, false otherwise.
//!
bool 
hw_mpu_region_get(
    unsigned int region, 
    uintptr_t* addr, 
    size_t* sz, 
    unsigned int* attr)
{
    bool enabled;
    uint32_t attributes;
    HW_MPU->region_number = region & 0xFF;
    attributes = HW_MPU->region_attr;    
    enabled = (attributes & 1) == 1;
    *addr = HW_MPU->region_base;
    *sz = 1 << (((attributes >> HW_MPU_RGN_SIZE) & 0x1F) + 1);
    *attr = (attributes >> HW_MPU_RGN_ATTRIB) & 0x7;
    return enabled;
}
