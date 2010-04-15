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

#ifndef __cplusplus

#define atomic_fetch_add_explicit( __a__, __m__, __x__ ) \
_ATOMIC_MODIFY_( __a__, ops_add, __m__, __x__ )

#define atomic_fetch_add( __a__, __m__ ) \
_ATOMIC_MODIFY_( __a__, ops_add, __m__, memory_order_seq_cst )


#define atomic_fetch_sub_explicit( __a__, __m__, __x__ ) \
_ATOMIC_MODIFY_( __a__, ops_sub, __m__, __x__ )

#define atomic_fetch_sub( __a__, __m__ ) \
_ATOMIC_MODIFY_( __a__, ops_sub, __m__, memory_order_seq_cst )


#define atomic_fetch_and_explicit( __a__, __m__, __x__ ) \
_ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ )

#define atomic_fetch_and( __a__, __m__ ) \
_ATOMIC_MODIFY_( __a__, ops_and, __m__, memory_order_seq_cst )


#define atomic_fetch_or_explicit( __a__, __m__, __x__ ) \
_ATOMIC_MODIFY_( __a__, ops_or, __m__, __x__ )

#define atomic_fetch_or( __a__, __m__ ) \
_ATOMIC_MODIFY_( __a__, ops_or, __m__, memory_order_seq_cst )


#define atomic_fetch_xor_explicit( __a__, __m__, __x__ ) \
_ATOMIC_MODIFY_( __a__, ops_xor, __m__, __x__ )

#define atomic_fetch_xor( __a__, __m__ ) \
_ATOMIC_MODIFY_( __a__, ops_xor, __m__, memory_order_seq_cst )

#else

inline void* atomic_fetch_add_explicit
    ( volatile atomic_address* __a__, ptrdiff_t __m__, memory_order __x__ )
{ 
    void* volatile* __p__ = &((__a__)->__f__);
    volatile atomic_flag* __g__ = __atomic_flag_for_address__( __p__ );
    __atomic_flag_wait_explicit__( __g__, __x__ );
    void* __r__ = *__p__;
    *__p__ = (void*)((char*)(*__p__) + __m__);
    atomic_flag_clear_explicit( __g__, __x__ );
    return __r__; 
}

inline void* atomic_fetch_add
( volatile atomic_address* __a__, ptrdiff_t __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline void* atomic_fetch_sub_explicit
( volatile atomic_address* __a__, ptrdiff_t __m__, memory_order __x__ )
{
    void* volatile* __p__ = &((__a__)->__f__);
    volatile atomic_flag* __g__ = __atomic_flag_for_address__( __p__ );
    __atomic_flag_wait_explicit__( __g__, __x__ );
    void* __r__ = *__p__;
    *__p__ = (void*)((char*)(*__p__) - __m__);
    atomic_flag_clear_explicit( __g__, __x__ );
    return __r__; 
}

inline void* atomic_fetch_sub
( volatile atomic_address* __a__, ptrdiff_t __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline char atomic_fetch_add_explicit
( volatile atomic_char* __a__, char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_add, __m__, __x__ ); }

inline char atomic_fetch_add
( volatile atomic_char* __a__, char __m__ )
{
    return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); 
}


inline char atomic_fetch_sub_explicit
( volatile atomic_char* __a__, char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_sub, __m__, __x__ ); }

inline char atomic_fetch_sub
( volatile atomic_char* __a__, char __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline char atomic_fetch_and_explicit
( volatile atomic_char* __a__, char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ ); }

inline char atomic_fetch_and
( volatile atomic_char* __a__, char __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline char atomic_fetch_or_explicit
( volatile atomic_char* __a__, char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_or, __m__, __x__ ); }

inline char atomic_fetch_or
( volatile atomic_char* __a__, char __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline char atomic_fetch_xor_explicit
( volatile atomic_char* __a__, char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ ); }

inline char atomic_fetch_xor
( volatile atomic_char* __a__, char __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline signed char atomic_fetch_add_explicit
( volatile atomic_schar* __a__, signed char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_add, __m__, __x__ ); }

inline signed char atomic_fetch_add
( volatile atomic_schar* __a__, signed char __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline signed char atomic_fetch_sub_explicit
( volatile atomic_schar* __a__, signed char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_sub, __m__, __x__ ); }

inline signed char atomic_fetch_sub
( volatile atomic_schar* __a__, signed char __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline signed char atomic_fetch_and_explicit
( volatile atomic_schar* __a__, signed char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ ); }

inline signed char atomic_fetch_and
( volatile atomic_schar* __a__, signed char __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline signed char atomic_fetch_or_explicit
( volatile atomic_schar* __a__, signed char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_or, __m__, __x__ ); }

inline signed char atomic_fetch_or
( volatile atomic_schar* __a__, signed char __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline signed char atomic_fetch_xor_explicit
( volatile atomic_schar* __a__, signed char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_xor, __m__, __x__ ); }

inline signed char atomic_fetch_xor
( volatile atomic_schar* __a__, signed char __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned char atomic_fetch_add_explicit
( volatile atomic_uchar* __a__, unsigned char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_add, __m__, __x__ ); }

inline unsigned char atomic_fetch_add
( volatile atomic_uchar* __a__, unsigned char __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned char atomic_fetch_sub_explicit
( volatile atomic_uchar* __a__, unsigned char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_sub, __m__, __x__ ); }

inline unsigned char atomic_fetch_sub
( volatile atomic_uchar* __a__, unsigned char __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned char atomic_fetch_and_explicit
( volatile atomic_uchar* __a__, unsigned char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ ); }

inline unsigned char atomic_fetch_and
( volatile atomic_uchar* __a__, unsigned char __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned char atomic_fetch_or_explicit
( volatile atomic_uchar* __a__, unsigned char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_or, __m__, __x__ ); }

inline unsigned char atomic_fetch_or
( volatile atomic_uchar* __a__, unsigned char __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned char atomic_fetch_xor_explicit
( volatile atomic_uchar* __a__, unsigned char __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_xor, __m__, __x__ ); }

inline unsigned char atomic_fetch_xor
( volatile atomic_uchar* __a__, unsigned char __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline short atomic_fetch_add_explicit
( volatile atomic_short* __a__, short __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_add, __m__, __x__ ); }

inline short atomic_fetch_add
( volatile atomic_short* __a__, short __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline short atomic_fetch_sub_explicit
( volatile atomic_short* __a__, short __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_sub, __m__, __x__ ); }

inline short atomic_fetch_sub
( volatile atomic_short* __a__, short __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline short atomic_fetch_and_explicit
( volatile atomic_short* __a__, short __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ ); }

inline short atomic_fetch_and
( volatile atomic_short* __a__, short __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline short atomic_fetch_or_explicit
( volatile atomic_short* __a__, short __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_or, __m__, __x__ ); }

inline short atomic_fetch_or
( volatile atomic_short* __a__, short __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline short atomic_fetch_xor_explicit
( volatile atomic_short* __a__, short __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_xor, __m__, __x__ ); }

inline short atomic_fetch_xor
( volatile atomic_short* __a__, short __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned short atomic_fetch_add_explicit
( volatile atomic_ushort* __a__, unsigned short __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_add, __m__, __x__ ); }

inline unsigned short atomic_fetch_add
( volatile atomic_ushort* __a__, unsigned short __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned short atomic_fetch_sub_explicit
( volatile atomic_ushort* __a__, unsigned short __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_sub, __m__, __x__ ); }

inline unsigned short atomic_fetch_sub
( volatile atomic_ushort* __a__, unsigned short __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned short atomic_fetch_and_explicit
( volatile atomic_ushort* __a__, unsigned short __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ ); }

inline unsigned short atomic_fetch_and
( volatile atomic_ushort* __a__, unsigned short __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned short atomic_fetch_or_explicit
( volatile atomic_ushort* __a__, unsigned short __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_or, __m__, __x__ ); }

inline unsigned short atomic_fetch_or
( volatile atomic_ushort* __a__, unsigned short __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned short atomic_fetch_xor_explicit
( volatile atomic_ushort* __a__, unsigned short __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ ); }

inline unsigned short atomic_fetch_xor
( volatile atomic_ushort* __a__, unsigned short __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline int atomic_fetch_add_explicit
( volatile atomic_int* __a__, int __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_add, __m__, __x__ ); }

inline int atomic_fetch_add
( volatile atomic_int* __a__, int __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline int atomic_fetch_sub_explicit
( volatile atomic_int* __a__, int __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_sub, __m__, __x__ ); }

inline int atomic_fetch_sub
( volatile atomic_int* __a__, int __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline int atomic_fetch_and_explicit
( volatile atomic_int* __a__, int __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ ); }

inline int atomic_fetch_and
( volatile atomic_int* __a__, int __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline int atomic_fetch_or_explicit
( volatile atomic_int* __a__, int __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_or, __m__, __x__ ); }

inline int atomic_fetch_or
( volatile atomic_int* __a__, int __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline int atomic_fetch_xor_explicit
( volatile atomic_int* __a__, int __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_xor, __m__, __x__ ); }

inline int atomic_fetch_xor
( volatile atomic_int* __a__, int __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned int atomic_fetch_add_explicit
( volatile atomic_uint* __a__, unsigned int __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_add, __m__, __x__ ); }

inline unsigned int atomic_fetch_add
( volatile atomic_uint* __a__, unsigned int __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned int atomic_fetch_sub_explicit
( volatile atomic_uint* __a__, unsigned int __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_sub, __m__, __x__ ); }

inline unsigned int atomic_fetch_sub
( volatile atomic_uint* __a__, unsigned int __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned int atomic_fetch_and_explicit
( volatile atomic_uint* __a__, unsigned int __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ ); }

inline unsigned int atomic_fetch_and
( volatile atomic_uint* __a__, unsigned int __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned int atomic_fetch_or_explicit
( volatile atomic_uint* __a__, unsigned int __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_or, __m__, __x__ ); }

inline unsigned int atomic_fetch_or
( volatile atomic_uint* __a__, unsigned int __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned int atomic_fetch_xor_explicit
( volatile atomic_uint* __a__, unsigned int __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_xor, __m__, __x__ ); }

inline unsigned int atomic_fetch_xor
( volatile atomic_uint* __a__, unsigned int __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long atomic_fetch_add_explicit
( volatile atomic_long* __a__, long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_add, __m__, __x__ ); }

inline long atomic_fetch_add
( volatile atomic_long* __a__, long __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long atomic_fetch_sub_explicit
( volatile atomic_long* __a__, long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_sub, __m__, __x__ ); }

inline long atomic_fetch_sub
( volatile atomic_long* __a__, long __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long atomic_fetch_and_explicit
( volatile atomic_long* __a__, long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ ); }

inline long atomic_fetch_and
( volatile atomic_long* __a__, long __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long atomic_fetch_or_explicit
( volatile atomic_long* __a__, long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_or, __m__, __x__ ); }

inline long atomic_fetch_or
( volatile atomic_long* __a__, long __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long atomic_fetch_xor_explicit
( volatile atomic_long* __a__, long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_xor, __m__, __x__ ); }

inline long atomic_fetch_xor
( volatile atomic_long* __a__, long __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long atomic_fetch_add_explicit
( volatile atomic_ulong* __a__, unsigned long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_add, __m__, __x__ ); }

inline unsigned long atomic_fetch_add
( volatile atomic_ulong* __a__, unsigned long __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long atomic_fetch_sub_explicit
( volatile atomic_ulong* __a__, unsigned long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_sub, __m__, __x__ ); }

inline unsigned long atomic_fetch_sub
( volatile atomic_ulong* __a__, unsigned long __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long atomic_fetch_and_explicit
( volatile atomic_ulong* __a__, unsigned long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ ); }

inline unsigned long atomic_fetch_and
( volatile atomic_ulong* __a__, unsigned long __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long atomic_fetch_or_explicit
( volatile atomic_ulong* __a__, unsigned long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_xor, __m__, __x__ ); }

inline unsigned long atomic_fetch_or
( volatile atomic_ulong* __a__, unsigned long __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long atomic_fetch_xor_explicit
( volatile atomic_ulong* __a__, unsigned long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_xor, __m__, __x__ ); }

inline unsigned long atomic_fetch_xor
( volatile atomic_ulong* __a__, unsigned long __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long long atomic_fetch_add_explicit
( volatile atomic_llong* __a__, long long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_add, __m__, __x__ ); }

inline long long atomic_fetch_add
( volatile atomic_llong* __a__, long long __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long long atomic_fetch_sub_explicit
( volatile atomic_llong* __a__, long long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_sub, __m__, __x__ ); }

inline long long atomic_fetch_sub
( volatile atomic_llong* __a__, long long __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long long atomic_fetch_and_explicit
( volatile atomic_llong* __a__, long long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ ); }

inline long long atomic_fetch_and
( volatile atomic_llong* __a__, long long __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long long atomic_fetch_or_explicit
( volatile atomic_llong* __a__, long long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_or, __m__, __x__ ); }

inline long long atomic_fetch_or
( volatile atomic_llong* __a__, long long __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long long atomic_fetch_xor_explicit
( volatile atomic_llong* __a__, long long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_xor, __m__, __x__ ); }

inline long long atomic_fetch_xor
( volatile atomic_llong* __a__, long long __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long long atomic_fetch_add_explicit
( volatile atomic_ullong* __a__, unsigned long long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_add, __m__, __x__ ); }

inline unsigned long long atomic_fetch_add
( volatile atomic_ullong* __a__, unsigned long long __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long long atomic_fetch_sub_explicit
( volatile atomic_ullong* __a__, unsigned long long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_sub, __m__, __x__ ); }

inline unsigned long long atomic_fetch_sub
( volatile atomic_ullong* __a__, unsigned long long __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long long atomic_fetch_and_explicit
( volatile atomic_ullong* __a__, unsigned long long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ ); }

inline unsigned long long atomic_fetch_and
( volatile atomic_ullong* __a__, unsigned long long __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long long atomic_fetch_or_explicit
( volatile atomic_ullong* __a__, unsigned long long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_or, __m__, __x__ ); }

inline unsigned long long atomic_fetch_or
( volatile atomic_ullong* __a__, unsigned long long __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long long atomic_fetch_xor_explicit
( volatile atomic_ullong* __a__, unsigned long long __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_xor, __m__, __x__ ); }

inline unsigned long long atomic_fetch_xor
( volatile atomic_ullong* __a__, unsigned long long __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline wchar_t atomic_fetch_add_explicit
( volatile atomic_wchar_t* __a__, wchar_t __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_add, __m__, __x__ ); }

inline wchar_t atomic_fetch_add
( volatile atomic_wchar_t* __a__, wchar_t __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline wchar_t atomic_fetch_sub_explicit
( volatile atomic_wchar_t* __a__, wchar_t __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_sub, __m__, __x__ ); }

inline wchar_t atomic_fetch_sub
( volatile atomic_wchar_t* __a__, wchar_t __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline wchar_t atomic_fetch_and_explicit
( volatile atomic_wchar_t* __a__, wchar_t __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_and, __m__, __x__ ); }

inline wchar_t atomic_fetch_and
( volatile atomic_wchar_t* __a__, wchar_t __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline wchar_t atomic_fetch_or_explicit
( volatile atomic_wchar_t* __a__, wchar_t __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_or, __m__, __x__ ); }

inline wchar_t atomic_fetch_or
( volatile atomic_wchar_t* __a__, wchar_t __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline wchar_t atomic_fetch_xor_explicit
( volatile atomic_wchar_t* __a__, wchar_t __m__, memory_order __x__ )
{ return _ATOMIC_MODIFY_( __a__, ops_xor, __m__, __x__ ); }

inline wchar_t atomic_fetch_xor
( volatile atomic_wchar_t* __a__, wchar_t __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }

#endif

