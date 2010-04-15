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


//supposed to be compiled by GNUC compiler
#if !defined(__GNUC__)
#error "This file must be compiled with GNUC compiler"
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 0)
#else
#error "GCC version too low. It must be greater than 4.0"
#endif


/* GCC builtins: http://gcc.gnu.org/onlinedocs/gcc/Atomic-Builtins.html
 * type __sync_fetch_and_add(type* ptr, type value)
 * type __sync_fetch_and_sub(type* ptr, type value)
 * type __sync_fetch_and_or(type* ptr, type value)
 * type __sync_fetch_and_and(type* ptr, type value)
 * type __sync_fetch_and_xor(type* ptr, type value)
 * type __sync_fetch_and_nand(type* ptr, type value)
 * 
 * type __sync_add_and_fetch(type* ptr, type value)
 * type __sync_sub_and_fetch(type* ptr, type value)
 * type __sync_or_and_fetch(type* ptr, type value)
 * type __sync_and_and_fetch(type* ptr, type value)
 * type __sync_xor_and_fetch(type* ptr, type value)
 * type __sync_nand_and_fetch(type* ptr, type value)
 */   
#ifdef __cplusplus

//FIXME: current implement doesn't consider the memory_order argument.
//full memory barrier is always implied

inline void* atomic_fetch_add_explicit
( volatile atomic_address* __a__, ptrdiff_t __m__, memory_order __x__ )
{return __sync_fetch_and_add(&__a__->__f__, __m__);}

inline void* atomic_fetch_add
( volatile atomic_address* __a__, ptrdiff_t __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline void* atomic_fetch_sub_explicit
( volatile atomic_address* __a__, ptrdiff_t __m__, memory_order __x__ )
{ return __sync_fetch_and_sub(&__a__->__f__, __m__);}

inline void* atomic_fetch_sub
( volatile atomic_address* __a__, ptrdiff_t __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline char atomic_fetch_add_explicit
( volatile atomic_char* __a__, char __m__, memory_order __x__ )
{ return __sync_fetch_and_add(&__a__->__f__, __m__);}

inline char atomic_fetch_add
( volatile atomic_char* __a__, char __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline char atomic_fetch_sub_explicit
( volatile atomic_char* __a__, char __m__, memory_order __x__ )
{ return __sync_fetch_and_sub(&__a__->__f__, __m__); }

inline char atomic_fetch_sub
( volatile atomic_char* __a__, char __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline char atomic_fetch_and_explicit
( volatile atomic_char* __a__, char __m__, memory_order __x__ )
{ return __sync_fetch_and_and(&__a__->__f__, __m__); }

inline char atomic_fetch_and
( volatile atomic_char* __a__, char __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline char atomic_fetch_or_explicit
( volatile atomic_char* __a__, char __m__, memory_order __x__ )
{ return __sync_fetch_and_or(&__a__->__f__, __m__); }

inline char atomic_fetch_or
( volatile atomic_char* __a__, char __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline char atomic_fetch_xor_explicit
( volatile atomic_char* __a__, char __m__, memory_order __x__ )
{ return __sync_fetch_and_xor(&__a__->__f__, __m__); }

inline char atomic_fetch_xor
( volatile atomic_char* __a__, char __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline signed char atomic_fetch_add_explicit
( volatile atomic_schar* __a__, signed char __m__, memory_order __x__ )
{ return __sync_fetch_and_add(&__a__->__f__, __m__); }

inline signed char atomic_fetch_add
( volatile atomic_schar* __a__, signed char __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline signed char atomic_fetch_sub_explicit
( volatile atomic_schar* __a__, signed char __m__, memory_order __x__ )
{ return __sync_fetch_and_sub(&__a__->__f__, __m__); }

inline signed char atomic_fetch_sub
( volatile atomic_schar* __a__, signed char __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline signed char atomic_fetch_and_explicit
( volatile atomic_schar* __a__, signed char __m__, memory_order __x__ )
{ return __sync_fetch_and_and(&__a__->__f__, __m__); }

inline signed char atomic_fetch_and
( volatile atomic_schar* __a__, signed char __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline signed char atomic_fetch_or_explicit
( volatile atomic_schar* __a__, signed char __m__, memory_order __x__ )
{ return __sync_fetch_and_or(&__a__->__f__, __m__); }

inline signed char atomic_fetch_or
( volatile atomic_schar* __a__, signed char __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline signed char atomic_fetch_xor_explicit
( volatile atomic_schar* __a__, signed char __m__, memory_order __x__ )
{ return __sync_fetch_and_xor(&__a__->__f__, __m__); }

inline signed char atomic_fetch_xor
( volatile atomic_schar* __a__, signed char __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned char atomic_fetch_add_explicit
( volatile atomic_uchar* __a__, unsigned char __m__, memory_order __x__ )
{ return __sync_fetch_and_add(&__a__->__f__, __m__); }

inline unsigned char atomic_fetch_add
( volatile atomic_uchar* __a__, unsigned char __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned char atomic_fetch_sub_explicit
( volatile atomic_uchar* __a__, unsigned char __m__, memory_order __x__ )
{ return __sync_fetch_and_sub(&__a__->__f__, __m__); }

inline unsigned char atomic_fetch_sub
( volatile atomic_uchar* __a__, unsigned char __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned char atomic_fetch_and_explicit
( volatile atomic_uchar* __a__, unsigned char __m__, memory_order __x__ )
{ return __sync_fetch_and_and(&__a__->__f__, __m__); }

inline unsigned char atomic_fetch_and
( volatile atomic_uchar* __a__, unsigned char __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned char atomic_fetch_or_explicit
( volatile atomic_uchar* __a__, unsigned char __m__, memory_order __x__ )
{ return __sync_fetch_and_or(&__a__->__f__, __m__); }

inline unsigned char atomic_fetch_or
( volatile atomic_uchar* __a__, unsigned char __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned char atomic_fetch_xor_explicit
( volatile atomic_uchar* __a__, unsigned char __m__, memory_order __x__ )
{ return __sync_fetch_and_xor(&__a__->__f__, __m__); }

inline unsigned char atomic_fetch_xor
( volatile atomic_uchar* __a__, unsigned char __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline short atomic_fetch_add_explicit
( volatile atomic_short* __a__, short __m__, memory_order __x__ )
{ return __sync_fetch_and_add(&__a__->__f__, __m__); }

inline short atomic_fetch_add
( volatile atomic_short* __a__, short __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline short atomic_fetch_sub_explicit
( volatile atomic_short* __a__, short __m__, memory_order __x__ )
{ return __sync_fetch_and_sub(&__a__->__f__, __m__); }

inline short atomic_fetch_sub
( volatile atomic_short* __a__, short __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline short atomic_fetch_and_explicit
( volatile atomic_short* __a__, short __m__, memory_order __x__ )
{ return __sync_fetch_and_and(&__a__->__f__, __m__); }

inline short atomic_fetch_and
( volatile atomic_short* __a__, short __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline short atomic_fetch_or_explicit
( volatile atomic_short* __a__, short __m__, memory_order __x__ )
{ return __sync_fetch_and_or(&__a__->__f__, __m__); }

inline short atomic_fetch_or
( volatile atomic_short* __a__, short __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline short atomic_fetch_xor_explicit
( volatile atomic_short* __a__, short __m__, memory_order __x__ )
{ return __sync_fetch_and_xor(&__a__->__f__, __m__); }

inline short atomic_fetch_xor
( volatile atomic_short* __a__, short __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned short atomic_fetch_add_explicit
( volatile atomic_ushort* __a__, unsigned short __m__, memory_order __x__ )
{ return __sync_fetch_and_add(&__a__->__f__, __m__); }

inline unsigned short atomic_fetch_add
( volatile atomic_ushort* __a__, unsigned short __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned short atomic_fetch_sub_explicit
( volatile atomic_ushort* __a__, unsigned short __m__, memory_order __x__ )
{ return __sync_fetch_and_sub(&__a__->__f__, __m__); }

inline unsigned short atomic_fetch_sub
( volatile atomic_ushort* __a__, unsigned short __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned short atomic_fetch_and_explicit
( volatile atomic_ushort* __a__, unsigned short __m__, memory_order __x__ )
{ return __sync_fetch_and_and(&__a__->__f__, __m__); }

inline unsigned short atomic_fetch_and
( volatile atomic_ushort* __a__, unsigned short __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned short atomic_fetch_or_explicit
( volatile atomic_ushort* __a__, unsigned short __m__, memory_order __x__ )
{ return __sync_fetch_and_or(&__a__->__f__, __m__); }

inline unsigned short atomic_fetch_or
( volatile atomic_ushort* __a__, unsigned short __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned short atomic_fetch_xor_explicit
( volatile atomic_ushort* __a__, unsigned short __m__, memory_order __x__ )
{ return __sync_fetch_and_xor(&__a__->__f__, __m__); }

inline unsigned short atomic_fetch_xor
( volatile atomic_ushort* __a__, unsigned short __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline int atomic_fetch_add_explicit
( volatile atomic_int* __a__, int __m__, memory_order __x__ )
{ return __sync_fetch_and_add(&__a__->__f__, __m__); }

inline int atomic_fetch_add
( volatile atomic_int* __a__, int __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline int atomic_fetch_sub_explicit
( volatile atomic_int* __a__, int __m__, memory_order __x__ )
{ return __sync_fetch_and_sub(&__a__->__f__, __m__); }

inline int atomic_fetch_sub
( volatile atomic_int* __a__, int __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline int atomic_fetch_and_explicit
( volatile atomic_int* __a__, int __m__, memory_order __x__ )
{ return __sync_fetch_and_and(&__a__->__f__, __m__); }

inline int atomic_fetch_and
( volatile atomic_int* __a__, int __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline int atomic_fetch_or_explicit
( volatile atomic_int* __a__, int __m__, memory_order __x__ )
{ return __sync_fetch_and_or(&__a__->__f__, __m__); }

inline int atomic_fetch_or
( volatile atomic_int* __a__, int __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline int atomic_fetch_xor_explicit
( volatile atomic_int* __a__, int __m__, memory_order __x__ )
{ return __sync_fetch_and_xor(&__a__->__f__, __m__); }

inline int atomic_fetch_xor
( volatile atomic_int* __a__, int __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned int atomic_fetch_add_explicit
( volatile atomic_uint* __a__, unsigned int __m__, memory_order __x__ )
{ return __sync_fetch_and_add(&__a__->__f__, __m__); }

inline unsigned int atomic_fetch_add
( volatile atomic_uint* __a__, unsigned int __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned int atomic_fetch_sub_explicit
( volatile atomic_uint* __a__, unsigned int __m__, memory_order __x__ )
{ return __sync_fetch_and_sub(&__a__->__f__, __m__); }

inline unsigned int atomic_fetch_sub
( volatile atomic_uint* __a__, unsigned int __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned int atomic_fetch_and_explicit
( volatile atomic_uint* __a__, unsigned int __m__, memory_order __x__ )
{ return __sync_fetch_and_and(&__a__->__f__, __m__); }

inline unsigned int atomic_fetch_and
( volatile atomic_uint* __a__, unsigned int __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned int atomic_fetch_or_explicit
( volatile atomic_uint* __a__, unsigned int __m__, memory_order __x__ )
{ return __sync_fetch_and_or(&__a__->__f__, __m__); }

inline unsigned int atomic_fetch_or
( volatile atomic_uint* __a__, unsigned int __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned int atomic_fetch_xor_explicit
( volatile atomic_uint* __a__, unsigned int __m__, memory_order __x__ )
{ return __sync_fetch_and_xor(&__a__->__f__, __m__); }

inline unsigned int atomic_fetch_xor
( volatile atomic_uint* __a__, unsigned int __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long atomic_fetch_add_explicit
( volatile atomic_long* __a__, long __m__, memory_order __x__ )
{ return __sync_fetch_and_add(&__a__->__f__, __m__); }

inline long atomic_fetch_add
( volatile atomic_long* __a__, long __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long atomic_fetch_sub_explicit
( volatile atomic_long* __a__, long __m__, memory_order __x__ )
{ return __sync_fetch_and_sub(&__a__->__f__, __m__); }

inline long atomic_fetch_sub
( volatile atomic_long* __a__, long __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long atomic_fetch_and_explicit
( volatile atomic_long* __a__, long __m__, memory_order __x__ )
{ return __sync_fetch_and_and(&__a__->__f__, __m__); }

inline long atomic_fetch_and
( volatile atomic_long* __a__, long __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long atomic_fetch_or_explicit
( volatile atomic_long* __a__, long __m__, memory_order __x__ )
{ return __sync_fetch_and_or(&__a__->__f__, __m__); }

inline long atomic_fetch_or
( volatile atomic_long* __a__, long __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long atomic_fetch_xor_explicit
( volatile atomic_long* __a__, long __m__, memory_order __x__ )
{ return __sync_fetch_and_xor(&(__a__->__f__), __m__); }

inline long atomic_fetch_xor
( volatile atomic_long* __a__, long __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long atomic_fetch_add_explicit
( volatile atomic_ulong* __a__, unsigned long __m__, memory_order __x__ )
{ return __sync_fetch_and_add(&(__a__->__f__), __m__); }

inline unsigned long atomic_fetch_add
( volatile atomic_ulong* __a__, unsigned long __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long atomic_fetch_sub_explicit
( volatile atomic_ulong* __a__, unsigned long __m__, memory_order __x__ )
{ return __sync_fetch_and_sub(&__a__->__f__, __m__); }

inline unsigned long atomic_fetch_sub
( volatile atomic_ulong* __a__, unsigned long __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long atomic_fetch_and_explicit
( volatile atomic_ulong* __a__, unsigned long __m__, memory_order __x__ )
{ return __sync_fetch_and_and(&__a__->__f__, __m__); }

inline unsigned long atomic_fetch_and
( volatile atomic_ulong* __a__, unsigned long __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long atomic_fetch_or_explicit
( volatile atomic_ulong* __a__, unsigned long __m__, memory_order __x__ )
{ return __sync_fetch_and_or(&__a__->__f__, __m__); }

inline unsigned long atomic_fetch_or
( volatile atomic_ulong* __a__, unsigned long __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long atomic_fetch_xor_explicit
( volatile atomic_ulong* __a__, unsigned long __m__, memory_order __x__ )
{ return __sync_fetch_and_xor(&__a__->__f__, __m__); }

inline unsigned long atomic_fetch_xor
( volatile atomic_ulong* __a__, unsigned long __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long long atomic_fetch_add_explicit
( volatile atomic_llong* __a__, long long __m__, memory_order __x__ )
{ return __sync_fetch_and_add(&__a__->__f__, __m__); }

inline long long atomic_fetch_add
( volatile atomic_llong* __a__, long long __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long long atomic_fetch_sub_explicit
( volatile atomic_llong* __a__, long long __m__, memory_order __x__ )
{ return __sync_fetch_and_sub(&__a__->__f__, __m__); }

inline long long atomic_fetch_sub
( volatile atomic_llong* __a__, long long __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long long atomic_fetch_and_explicit
( volatile atomic_llong* __a__, long long __m__, memory_order __x__ )
{ return __sync_fetch_and_and(&__a__->__f__, __m__); }

inline long long atomic_fetch_and
( volatile atomic_llong* __a__, long long __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long long atomic_fetch_or_explicit
( volatile atomic_llong* __a__, long long __m__, memory_order __x__ )
{ return __sync_fetch_and_or(&__a__->__f__, __m__); }

inline long long atomic_fetch_or
( volatile atomic_llong* __a__, long long __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline long long atomic_fetch_xor_explicit
( volatile atomic_llong* __a__, long long __m__, memory_order __x__ )
{ return __sync_fetch_and_xor(&__a__->__f__, __m__); }

inline long long atomic_fetch_xor
( volatile atomic_llong* __a__, long long __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long long atomic_fetch_add_explicit
( volatile atomic_ullong* __a__, unsigned long long __m__, memory_order __x__ )
{ return __sync_fetch_and_add(&__a__->__f__, __m__); }

inline unsigned long long atomic_fetch_add
( volatile atomic_ullong* __a__, unsigned long long __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long long atomic_fetch_sub_explicit
( volatile atomic_ullong* __a__, unsigned long long __m__, memory_order __x__ )
{ return __sync_fetch_and_sub(&__a__->__f__, __m__); }

inline unsigned long long atomic_fetch_sub
( volatile atomic_ullong* __a__, unsigned long long __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long long atomic_fetch_and_explicit
( volatile atomic_ullong* __a__, unsigned long long __m__, memory_order __x__ )
{ return __sync_fetch_and_and(&__a__->__f__, __m__); }

inline unsigned long long atomic_fetch_and
( volatile atomic_ullong* __a__, unsigned long long __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long long atomic_fetch_or_explicit
( volatile atomic_ullong* __a__, unsigned long long __m__, memory_order __x__ )
{ return __sync_fetch_and_or(&__a__->__f__, __m__); }

inline unsigned long long atomic_fetch_or
( volatile atomic_ullong* __a__, unsigned long long __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline unsigned long long atomic_fetch_xor_explicit
( volatile atomic_ullong* __a__, unsigned long long __m__, memory_order __x__ )
{ return __sync_fetch_and_xor(&__a__->__f__, __m__); }

inline unsigned long long atomic_fetch_xor
( volatile atomic_ullong* __a__, unsigned long long __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }


inline wchar_t atomic_fetch_add_explicit
( volatile atomic_wchar_t* __a__, wchar_t __m__, memory_order __x__ )
{ return __sync_fetch_and_add(&__a__->__f__, __m__); }

inline wchar_t atomic_fetch_add
( volatile atomic_wchar_t* __a__, wchar_t __m__ )
{ return atomic_fetch_add_explicit( __a__, __m__, memory_order_seq_cst ); }


inline wchar_t atomic_fetch_sub_explicit
( volatile atomic_wchar_t* __a__, wchar_t __m__, memory_order __x__ )
{ return __sync_fetch_and_sub(&__a__->__f__, __m__); }

inline wchar_t atomic_fetch_sub
( volatile atomic_wchar_t* __a__, wchar_t __m__ )
{ return atomic_fetch_sub_explicit( __a__, __m__, memory_order_seq_cst ); }


inline wchar_t atomic_fetch_and_explicit
( volatile atomic_wchar_t* __a__, wchar_t __m__, memory_order __x__ )
{ return __sync_fetch_and_and(&__a__->__f__, __m__); }

inline wchar_t atomic_fetch_and
( volatile atomic_wchar_t* __a__, wchar_t __m__ )
{ return atomic_fetch_and_explicit( __a__, __m__, memory_order_seq_cst ); }


inline wchar_t atomic_fetch_or_explicit
( volatile atomic_wchar_t* __a__, wchar_t __m__, memory_order __x__ )
{ return __sync_fetch_and_or(&__a__->__f__, __m__); }

inline wchar_t atomic_fetch_or
( volatile atomic_wchar_t* __a__, wchar_t __m__ )
{ return atomic_fetch_or_explicit( __a__, __m__, memory_order_seq_cst ); }


inline wchar_t atomic_fetch_xor_explicit
( volatile atomic_wchar_t* __a__, wchar_t __m__, memory_order __x__ )
{ return __sync_fetch_and_xor(&__a__->__f__, __m__); }

inline wchar_t atomic_fetch_xor
( volatile atomic_wchar_t* __a__, wchar_t __m__ )
{ return atomic_fetch_xor_explicit( __a__, __m__, memory_order_seq_cst ); }

#else /* C macros */

#define atomic_fetch_add_explicit( __a__, __m__, __x__ ) \
__sync_fetch_and_add(&((__a__)->__f__), __m__)

#define atomic_fetch_add( __a__, __m__ ) \
__sync_fetch_and_add(&((__a__)->__f__), __m__)


#define atomic_fetch_sub_explicit( __a__, __m__, __x__ ) \
__sync_fetch_and_sub(&((__a__)->__f__), __m__)

#define atomic_fetch_sub( __a__, __m__ ) \
__sync_fetch_and_sub(&((__a__)->__f__), __m__)


#define atomic_fetch_and_explicit( __a__, __m__, __x__ ) \
__sync_fetch_and_and(&((__a__)->__f__), __m__)

#define atomic_fetch_and( __a__, __m__ ) \
__sync_fetch_and_and(&((__a__)->__f__), __m__)


#define atomic_fetch_or_explicit( __a__, __m__, __x__ ) \
__sync_fetch_and_or(&((__a__)->__f__), __m__)

#define atomic_fetch_or( __a__, __m__ ) \
__sync_fetch_and_or(&((__a__)->__f__), __m__)


#define atomic_fetch_xor_explicit( __a__, __m__, __x__ ) \
__sync_fetch_and_xor(&((__a__)->__f__), __m__)

#define atomic_fetch_xor( __a__, __m__ ) \
__sync_fetch_and_xor(&((__a__)->__f__), __m__)

#endif
/******** end ********************************************************/
