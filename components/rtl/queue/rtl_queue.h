#ifndef _RTL_QUEUE_V1_HEADER_
#define _RTL_QUEUE_V1_HEADER_

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

#include <stddef.h>

typedef struct _rtl_queue 
{
    struct _rtl_queue* next;
    struct _rtl_queue* prev;
}
rtl_queue_t, rtl_queue_linkage_t;

#define RTL_QUEUE_INITIALIZER {NULL, NULL}

#define rtl_queue_init(head) ((head)->next = (head)->prev = (head))
#define rtl_queue_empty(head) ((head)->next == (head))
#define rtl_queue_next(q) ((q)->next)
#define rtl_queue_prev(q) ((q)->prev)
#define rtl_queue_first(head) ((head)->next)
#define rtl_queue_last(head) ((head)->prev)
#define rtl_queue_end(head,q) ((q) == (head))
#define rtl_queue_is_item_linked(q) (((q)->next) && ((q)->prev))
#define rtl_queue_item_init(item) ((item)->next = (item)->prev = NULL)
#define rtl_queue_entry(q, type, member) \
    ((type*)((char*)(q) - (size_t)(&((type*)0)->member)))

void rtl_enqueue(rtl_queue_t*, rtl_queue_t*);
rtl_queue_t* rtl_dequeue(rtl_queue_t*);
void rtl_queue_insert(rtl_queue_t*, rtl_queue_t*);
void rtl_queue_remove(rtl_queue_t*);
void rtl_queue_copy(rtl_queue_t* dst, rtl_queue_t* src);

FX_METADATA(({ interface: [RTL_QUEUE, V1] }))

#endif
