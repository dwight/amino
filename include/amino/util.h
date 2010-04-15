/*
 * (c) Copyright 2008, IBM Corporation.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AMINO_UTIL_H
#define AMINO_UTIL_H

#include <amino/config.h>

#define FREELIST

/**
 *  This enum is used to specify operation used by 
 *  ATOMIC_MODIFY macro
 **/
typedef enum ops {
  ops_swap,ops_add,ops_sub,ops_and,ops_or,ops_xor
} ops;

#if defined(X86)
#include "arch/x86/x86_util.h"
#elif defined(PPC)
#include "arch/ppc/ppc_util.h"
#else
#warning Unsupported platform, generic version of atomic package will be used
#include "arch/platform_util.h"
#endif

#endif
