/** 
  ******************************************************************************
  *  @file   smp/hal_barrier.h
  *  @brief  SMP implementation of low-level active barriers.
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

#include FX_INTERFACE(HAL_BARRIER)

FX_METADATA(({ implementation: [HAL_BARRIER, SMP] }))

//!
//! Set barrier to specified value atomically.
//! @param [in] barrier Barrier to be set.
//! @param [in] newval New value of the barrier.
//!
void
hal_barrier_set(hal_barrier_t* barrier, unsigned newval)
{   
    //
    // It is expected that barrier_set acts like a release barrier, so, add 
    // memory barrier before setting the barrier value.
    //
    hw_cpu_dmb();
    barrier->lock = newval;
}

//!
//! Add specified value to the barrier value.
//! @param [in] barrier Barrier to be incremented.
//! @param [in] addend Value to be added to the barrier.
//!
void
hal_barrier_add(hal_barrier_t* barrier, unsigned addend)
{
    (void) hw_cpu_atomic_add(&barrier->lock, (unsigned) addend);
}

//!
//! Wait for barrier to be set in specified value.
//! @param [in] barrier Target barrier.
//! @param [in] key Expected value of barrier which satisfies the wait.
//!
void 
hal_barrier_wait(hal_barrier_t* barrier, unsigned key)
{
    while (barrier->lock != key) ;
    hw_cpu_dmb();
}

