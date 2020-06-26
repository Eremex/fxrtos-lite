/** 
  ******************************************************************************
  *  @file   mpq/fx_sched_alg.c
  *  @brief  Implementation of scheduler container.
  *  The containter is designed as two-level bitmap hierarchy plus array of
  *  queues corresponding to each 2nd level map bit.
  *  Each priority level has associated queue of items with the same priority. 
  *  Container should always contain at least one item.
  *  All container functions expect SPL = SCHED_LEVEL.
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

#include FX_INTERFACE(HW_CPU)
#include FX_INTERFACE(FX_SCHED_ALG)

FX_METADATA(({ implementation: [FX_SCHED_ALG, MPQ_FIFO] }))

//!
//! Constructor of a schedulable item.
//! @param [in,out] item Target schedulable item to be initialized.
//! @param [in] type Initialization type, see @ref fx_sched_params_init_t.
//! @param [in] src  Schedulable item which will be used as source of params in 
//! case when FX_SCHED_PARAMS_INIT_SPECIFIED specified as the type.
//! @remark src parameter cannot be NULL if type is 
//! FX_SCHED_PARAMS_INIT_SPECIFIED.
//!
void
fx_sched_params_init(
    fx_sched_params_t* item, 
    fx_sched_params_init_t type, 
    const fx_sched_params_t* src)
{
    switch (type)
    {
    case FX_SCHED_PARAMS_INIT_IDLE: 
        item->prio = FX_SCHED_ALG_PRIO_IDLE; break;
    case FX_SCHED_PARAMS_INIT_DEFAULT: 
        item->prio = FX_SCHED_ALG_PRIO_IDLE-1; break;
    case FX_SCHED_PARAMS_INIT_SPECIFIED: 
        item->prio = src->prio; break;
    };
}

//!
//! Constructor of scheduler container.
//! it is assumed that container memory is already filled by zero by the caller.
//! @param [in] container Pointer to uninitialized scheduler container.
//!
void 
fx_sched_container_init(fx_sched_container_t* container)
{
    unsigned int i = 0;
    for (i = 0; i < FX_SCHED_ALG_PRIO_NUM; ++i)
    {
        rtl_queue_init(&(container->priority_queues[i]));
    }
}

//
// Helper functions. It is assumed that the compiler is smart enough to replace 
// division by integer power of 2 to logical shifts.
//
#define div_int_sz(item) ((item)->prio / lang_type_to_bits(unsigned int))
#define mod_int_sz(item) ((item)->prio % lang_type_to_bits(unsigned int))

//!
//! Add item to container.
//! @param [in] container Target scheduler container.
//! @param [in] item Schedulable item to be added into the container.
//!
void 
fx_sched_container_add(fx_sched_container_t* container, fx_sched_params_t* item)
{
    rtl_enqueue(&(container->priority_queues[item->prio]), &item->link);
    container->map1 |= 1 << div_int_sz(item);
    container->map2[div_int_sz(item)] |= 1 << mod_int_sz(item);
}

//!
//! Remove item from container.
//! @param [in] container Target scheduler container.
//! @param [in] item Schedulable item to be added into the container.
//!
void 
fx_sched_container_remove(
    fx_sched_container_t* container, 
    fx_sched_params_t* item)
{
    rtl_queue_remove(&item->link);
    if (rtl_queue_empty(&(container->priority_queues[item->prio])))
    {
        const unsigned int updated_map2 = 
            container->map2[div_int_sz(item)] &= ~(1 << mod_int_sz(item));

        if (updated_map2 == 0)
        {
            container->map1 &= ~(1 << div_int_sz(item));
        }
    }
}

//!
//! Scheduling function.
//! It selects schedulable item with highest priority from items which are 
//! contained in the specified container.
//! @param [in] container Container where scheduing should be performed.
//! @return Scheduled item. 
//! @warning  Container must contain at least one element before this 
//! function call.
//!
fx_sched_params_t* 
fx_sched_container_get(fx_sched_container_t* container)
{
    rtl_queue_t* const queues = container->priority_queues;
    const unsigned int map1_index = hw_cpu_ctz(container->map1);
    const unsigned int map2_index = hw_cpu_ctz(container->map2[map1_index]);
    const unsigned int queue_index = 
        lang_type_to_bits(unsigned int) * map1_index + map2_index;

    return rtl_queue_entry(
        rtl_queue_first(&(queues[ queue_index ])), 
        fx_sched_params_t, 
        link
    );
}
