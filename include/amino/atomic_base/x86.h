/* Copyright 2008, IBM Corporation.
 * Licensed under the Apache License, Version 2.0 (the "License"); 
   you may not use this file except in compliance with the License. 
   You may obtain a copy of the License at 
   
        http://www.apache.org/licenses/LICENSE-2.0 
        
   Unless required by applicable law or agreed to in writing, software 
   distributed under the License is distributed on an "AS IS" BASIS, 
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
   See the License for the specific language governing permissions and 
   limitations under the License.
 *
 * Optimization of atomics for gcc-x64 & gcc-i386
 * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2427.html
 */

/*
 * The X86 hardware memory model is strong.
 * __x86_64__:
 * 1 Loads are not reordered with other loads
 * 2 Stores are not reordered with other stores
 * 3 Stores are not reordered with older loads
 * 4 Stores by a single processor are observed in the same order by all processors
 * 5 Writes from the individual processors on the system bus are NOT ordered with
 *     respect to each other
 * 6 Intra-processor forwarding is allowed, which allows stores by two processors
 *     to be seen in different orders by those two processors
 * ...
 *
 * just for GCC--LINUX64. no Intel compiler and win32
 * Attention: data type length is different between LINUX64 & WINDOWS64.
 *
 * Write combining memory type is not considered for now...
 */

/* Additional comments:
 * The library optimization cannot achieve good result as compiler. The ideal implementation is 
 * supposed to be taken by compiler. For example:
 * address-sensitive fence... what we can do now is just to generate an address-free fence.
 * It is best implemented by compiler, which can explore the potential optimization room. 
 */

#include <amino/util.h>


//supposed to be compiled by GNUC compiler
#if !defined(GCC)
#error "This file must be compiled with GNUC compiler"
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 0)
#else
#error "GCC version too low. It must be greater than 4.0"
#endif

/****************** SC atomic? what about write combining mode?**********************/
#define _ATOMIC_LOAD_( __a__, __x__ ) \
({\
   if(__x__ != memory_order_relaxed) \
       compiler_barrier();\
   __typeof__((__a__)->__f__) __r__ = (__a__)->__f__; \
   __r__; })

#define _ATOMIC_STORE_( __a__, __m__, __x__ ) \
({\
   if(__x__ != memory_order_relaxed) \
       compiler_barrier();\
   if(__x__ == memory_order_seq_cst)\
        __asm__ __volatile(\
            "XCHG %0, %1"\
            :"=q"(__a__->__f__)\
            :"m"(__m__)\
            );\
   else\
        (__a__)->__f__ = __m__; \
   __m__; })

/* address-sensitive fence. but we treat it as address-free fence, since library impl. */
#define _ATOMIC_FENCE_( __a__, __x__ ) \
({ if(__x__ == memory_order_acquire || __x__ == memory_order_release) \
       compiler_barrier(); \
   else if(__x__ == memory_order_seq_cst) \
       SC_FENCE; \
   else if(__x__ == memory_order_acq_rel) \
       CC_FENCE; \
   })

/* for x86, CAS implies strongest barrier */
#define _ATOMIC_CMPSWP_( __a__, __e__, __m__, __x__ ) \
({ \
   __typeof__(__m__) __v__ = (__m__); \
   bool __r__; \
   __r__ = cas(__a__, __e__, (unsigned long)__v__, sizeof(__v__)); \
   __r__; })

/* This is a general "modify" function. Specific funcs, such as fetch_and_add, can be
 * implemented by gcc builtin.
 */
#define _ATOMIC_MODIFY_( __a__, __o__, __m__, __x__ ) \
({ volatile __typeof__((__a__)->__f__)* __p__ = &((__a__)->__f__); \
   __typeof__(__m__) __oldValue__; \
   __typeof__((__a__)->__f__) __newValue__; \
   do { \
       __oldValue__ = *__p__; \
       __newValue__ = __oldValue__; \
       switch(__o__){\
    case ops_swap:\
       __newValue__ = __m__; \
       break; \
 case ops_add:\
       __newValue__ += __m__; \
       break; \
 case ops_sub: \
       __newValue__ -= __m__; \
       break; \
 case ops_and:\
       __newValue__ &= __m__; \
       break; \
 case ops_or:\
       __newValue__ |= __m__; \
       break; \
 case ops_xor:\
       __newValue__ ^= __m__; \
       break; \
       }\
   } while(!cas(__a__, &__oldValue__, (unsigned long)__newValue__, sizeof(__newValue__)));\
   __oldValue__; })
   
/*************************************************************************************/


/**
 * To facilitate optimal storage use, the proposal supplies a feature macro that indicates
 * general lock-free status of scalar atomic types.
 * A value of 0 indicates that the scalar types are never lock-free.
 * A value of 1 indicates that the scalar types are sometimes lock-free.
 * A value of 2 indicates that the scalar types are always lock-free.
 */
#define ATOMIC_INTEGRAL_LOCK_FREE 2
#define ATOMIC_ADDRESS_LOCK_FREE 2 


/************* atomic_flag ***********************************/
bool atomic_flag_test_and_set_explicit( 
        volatile atomic_flag* __a__, memory_order __x__ );

bool atomic_flag_test_and_set( volatile atomic_flag* __a__ );

void atomic_flag_clear_explicit( 
        volatile atomic_flag* __a__, memory_order __x__ );

void atomic_flag_clear(volatile atomic_flag* __a__ );

void atomic_flag_fence(const volatile atomic_flag* __a__, memory_order __x__ );
/************* end of atomic_flag ***********************************/

/**** implementation of atomic_flag *****/
#ifdef __cplusplus

inline bool atomic_flag::test_and_set( memory_order __x__ ) volatile
{ return atomic_flag_test_and_set_explicit( this, __x__ ); }

inline void atomic_flag::clear( memory_order __x__ ) volatile
{ atomic_flag_clear_explicit( this, __x__ ); }

inline void atomic_flag::fence( memory_order __x__ ) const volatile
{ atomic_flag_fence( this, __x__ ); }

#endif
