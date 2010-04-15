/*
 * (c) Copyright 2008, IBM Corporation.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef STD_ATOMIC_H
#define STD_ATOMIC_H

#include <amino/impatomic.h>

#ifdef __cplusplus


using std::atomic_flag;


using std::atomic_bool;


using std::atomic_address;


using std::atomic_char;


using std::atomic_schar;


using std::atomic_uchar;


using std::atomic_short;


using std::atomic_ushort;


using std::atomic_int;


using std::atomic_uint;


using std::atomic_long;


using std::atomic_ulong;


using std::atomic_llong;


using std::atomic_ullong;


using std::atomic_wchar_t;


using std::atomic;
using std::memory_order;
using std::memory_order_relaxed;
using std::memory_order_acquire;
using std::memory_order_release;
using std::memory_order_acq_rel;
using std::memory_order_seq_cst;

#endif
#endif
