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

#include <amino/util.h>

#ifdef __cplusplus
extern "C" {
#endif
    extern bool atomic_flag_test_and_set( volatile atomic_flag* );
    extern bool atomic_flag_test_and_set_explicit
        ( volatile atomic_flag*, memory_order );
    extern void atomic_flag_clear( volatile atomic_flag* );
    extern void atomic_flag_clear_explicit
        ( volatile atomic_flag*, memory_order );
    extern void atomic_flag_fence
        ( const volatile atomic_flag*, memory_order );
    extern void __atomic_flag_wait__
        ( volatile atomic_flag* );
    extern void __atomic_flag_wait_explicit__
        ( volatile atomic_flag*, memory_order );
    extern volatile atomic_flag* __atomic_flag_for_address__
        ( const volatile void* __z__ )
        __attribute__((const));

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

inline bool atomic_flag::test_and_set( memory_order __x__ ) volatile
{ return atomic_flag_test_and_set_explicit( this, __x__ ); }

inline void atomic_flag::clear( memory_order __x__ ) volatile
{ atomic_flag_clear_explicit( this, __x__ ); }

inline void atomic_flag::fence( memory_order __x__ ) const volatile
{ atomic_flag_fence( this, __x__ ); }

#endif

#define _ATOMIC_LOAD_( __a__, __x__ ) \
    ({ volatile __typeof__((__a__)->__f__)* __p__ = &((__a__)->__f__); \
     volatile atomic_flag* __g__ = __atomic_flag_for_address__( __p__ ); \
     __atomic_flag_wait_explicit__( __g__, __x__ ); \
     __typeof__((__a__)->__f__) __r__ = *__p__; \
     atomic_flag_clear_explicit( __g__, __x__ ); \
     __r__; })

#define _ATOMIC_STORE_( __a__, __m__, __x__ ) \
    ({ volatile __typeof__((__a__)->__f__)* __p__ = &((__a__)->__f__); \
     __typeof__(__m__) __v__ = (__m__); \
     volatile atomic_flag* __g__ = __atomic_flag_for_address__( __p__ ); \
     __atomic_flag_wait_explicit__( __g__, __x__ ); \
     *__p__ = __v__; \
     atomic_flag_clear_explicit( __g__, __x__ ); \
     __v__; })

#define _ATOMIC_MODIFY_( __a__, __o__, __m__, __x__ ) \
    ({ volatile __typeof__((__a__)->__f__)* __p__ = &((__a__)->__f__); \
     __typeof__(__m__) __v__ = (__m__); \
     volatile atomic_flag* __g__ = __atomic_flag_for_address__( __p__ ); \
     __atomic_flag_wait_explicit__( __g__, __x__ ); \
     __typeof__((__a__)->__f__) __r__ = *__p__; \
     switch(__o__){\
         case ops_swap:\
            *__p__ = __v__; \
            break; \
         case ops_add:\
               *__p__ += __v__; \
               break; \
         case ops_sub: \
               *__p__ -= __v__; \
               break; \
         case ops_and:\
               *__p__ &= __v__; \
               break; \
         case ops_or:\
               *__p__ |= __v__; \
               break; \
         case ops_xor:\
               *__p__ ^= __v__; \
               break; \
       }\
     atomic_flag_clear_explicit( __g__, __x__ ); \
     __r__; })

#define _ATOMIC_CMPSWP_( __a__, __e__, __m__, __x__ ) \
    ({ volatile __typeof__((__a__)->__f__)* __p__ = &((__a__)->__f__); \
     __typeof__(__e__) __q__ = (__e__); \
     __typeof__(__m__) __v__ = (__m__); \
     bool __r__; \
     volatile atomic_flag* __g__ = __atomic_flag_for_address__( __p__ ); \
     __atomic_flag_wait_explicit__( __g__, __x__ ); \
     __typeof__((__a__)->__f__) __t__ = *__p__; \
     if ( __t__ == *__q__ ) { *__p__ = __v__; __r__ = true; } \
     else { *__q__ = __t__; __r__ = false; } \
     atomic_flag_clear_explicit( __g__, __x__ ); \
     __r__; })

#define _ATOMIC_FENCE_( __a__, __x__ ) \
    ({ volatile __typeof__((__a__)->__f__)* __p__ = &((__a__)->__f__); \
     volatile atomic_flag* __g__ = __atomic_flag_for_address__( __p__ ); \
     atomic_flag_fence( __g__, __x__ ); \
     })

#define ATOMIC_INTEGRAL_LOCK_FREE 0
#define ATOMIC_ADDRESS_LOCK_FREE 0

void __atomic_flag_wait__( volatile atomic_flag* __a__ )
{ 
    while ( atomic_flag_test_and_set( __a__ ) ); 
}

void __atomic_flag_wait_explicit__( volatile atomic_flag* __a__,
        memory_order __x__ )
{ 
    while ( atomic_flag_test_and_set_explicit( __a__, __x__ ) ); 
}

#define LOGSIZE 4

static atomic_flag volatile private_flag_table[ 1 << LOGSIZE ] =
{
    {false}, {false}, {false}, {false},
    {false}, {false}, {false}, {false},
    {false}, {false}, {false}, {false},
    {false}, {false}, {false}, {false}
};

volatile atomic_flag* __atomic_flag_for_address__( const volatile void* __z__ )
{
    uintptr_t __u__ = (uintptr_t)__z__;
    __u__ ^= (__u__ >> 2) ^ (__u__ << 3);
    __u__ ^= (__u__ >> 7) ^ (__u__ << 9);
    __u__ ^= (__u__ >> 18) ^ (__u__ << 17);
    if ( sizeof(uintptr_t) > 4 ) __u__ += (__u__ >> 31);
    __u__ &= ~((~(uintptr_t)0) << LOGSIZE);
    return private_flag_table + __u__;
}

bool atomic_flag_test_and_set_explicit( 
    volatile atomic_flag* __a__, memory_order __x__ )
{
    bool result = __a__->__f__;
    __a__->__f__ = true;

    return result;
}

bool atomic_flag_test_and_set( volatile atomic_flag* __a__ )
{ 
    return atomic_flag_test_and_set_explicit( __a__, memory_order_seq_cst ); 
}

void atomic_flag_clear_explicit
    (volatile atomic_flag* __a__, memory_order __x__ )
{
    __a__->__f__ = false;
} 

void atomic_flag_clear( volatile atomic_flag* __a__ )
{ 
    atomic_flag_clear_explicit( __a__, memory_order_seq_cst ); 
}

void atomic_flag_fence( const volatile atomic_flag* __a__, memory_order __x__ )
{ 
} 


