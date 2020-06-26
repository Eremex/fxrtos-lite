/*
  * Copyright (c) 2005-2007, Kohsuke Ohtani
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions
  * are met:
  * 1. Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright
  *    notice, this list of conditions and the following disclaimer in the
  *    documentation and/or other materials provided with the distribution.
  * 3. Neither the name of the author nor the names of any co-contributors
  *    may be used to endorse or promote products derived from this software
  *    without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  * SUCH DAMAGE.
  */

#include FX_INTERFACE(RTL_QUEUE)

FX_METADATA(({ implementation: [RTL_QUEUE, V1] }))

//
// Insert item at end of the queue.
//
void
rtl_enqueue(rtl_queue_t* head, rtl_queue_t* item)
{
    item->next = head;
    item->prev = head->prev;
    item->prev->next = item;
    head->prev = item;
}

//
// Try get item from head of the queue. Return NULL if the queue is empty.
//
rtl_queue_t*
rtl_dequeue(rtl_queue_t* head)
{
    rtl_queue_t* item;

    if (head->next == head)
        return ((rtl_queue_t*)0);

    item = head->next;
    item->next->prev = head;
    head->next = item->next;
    item->prev = item->next = NULL;

    return item;
}

//
// Insert new item after "prev" item.
//
void
rtl_queue_insert(rtl_queue_t* prev, rtl_queue_t* item)
{
    item->prev = prev;
    item->next = prev->next;
    prev->next->prev = item;
    prev->next = item;
}

//
// Extract item from queue.
//
void
rtl_queue_remove(rtl_queue_t* item)
{
    item->prev->next = item->next;
    item->next->prev = item->prev;
    item->prev = item->next = NULL;
} 

//
// Copy items from one queue to another. The destination queue must be empty.
//
void 
rtl_queue_copy(rtl_queue_t* dst, rtl_queue_t* src)
{
    dst->prev->next = src->next;
    src->next->prev = dst->prev;
    dst->prev = src->prev;
    src->prev->next = dst;
}
