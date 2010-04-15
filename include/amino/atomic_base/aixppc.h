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

#include <amino/util.h>
#include <assert.h>

#define _ATOMIC_LOAD_( __a__, __x__ ) \
({ volatile __typeof__((__a__)->__f__) *__p__ = &((__a__)->__f__); \
   __typeof__((__a__)->__f__) __r__; \
   int size = sizeof(__r__); \
   switch(size) { \
   	 case 1: \
   	 	   if(__x__ == memory_order_seq_cst) \
   			  asm volatile ( \
            			      "        sync\n"); \
 		   if((__x__ == memory_order_acquire) || (__x__ == memory_order_seq_cst)) \
   			  asm volatile ( \
            			      "        lbz          %0,%1\n" \
      			              "        cmpw         %0,%0\n" \
          			      "        bne-          $-8\n" \
          			      "        isync\n" \
           			          : "=r"(__r__) \
          			          : "m"(*__p__) \
        			          : "memory", "cc"); \
	       else { \
  		      __r__ = *__p__; \
                } \
   	 break; \
   	 case 2: \
   	 	   if(__x__ == memory_order_seq_cst) \
   			  asm volatile ( \
            			      "        sync\n"); \
 		   if((__x__ == memory_order_acquire) || (__x__ == memory_order_seq_cst)) \
   			  asm volatile ( \
            			      "        lhz          %0,%1\n" \
      			              "        cmpw         %0,%0\n" \
          			      "        bne-          $-8\n" \
          			      "        isync\n" \
           			          : "=r"(__r__) \
          			          : "m"(*__p__) \
        			          : "memory", "cc"); \
	       else \
  		      __r__ = *__p__; \
   	 break; \
   	 case 4: \
   	 	   if(__x__ == memory_order_seq_cst) \
   			  asm volatile ( \
            			      "        sync\n"); \
 		   if((__x__ == memory_order_acquire) || (__x__ == memory_order_seq_cst)) \
   			  asm volatile ( \
            			      "        lwz          %0,%1\n" \
      			              "        cmpw         %0,%0\n" \
          			      "        bne-         $-8\n" \
          			      "        isync\n" \
           			          : "=r"(__r__) \
          			          : "m"(*__p__) \
        			          : "memory", "cc"); \
	       else \
  		      __r__ = *__p__; \
   	 break; \
   	 case 8: \
   	 	   if(__x__ == memory_order_seq_cst) \
   			  asm volatile ( \
            			      "        sync\n"); \
 		   if((__x__ == memory_order_acquire) || (__x__ == memory_order_seq_cst)) \
   			  asm volatile ( \
            			      "        ld           %0,%1\n" \
      			              "        cmpw         %0,%0\n" \
          			      "        bne-         $-8\n" \
          			      "        isync\n" \
           			          : "=r"(__r__) \
          			          : "m"(*__p__) \
        			          : "memory", "cc"); \
	       else \
  		      __r__ = *__p__; \
   	 break; \
   } \
   __r__; })


#define _ATOMIC_STORE_( __a__, __m__, __x__ ) \
({ volatile __typeof__((__a__)->__f__) *__p__ = &((__a__)->__f__); \
   __typeof__(__m__) __v__ = (__m__); \
   if(__x__ == memory_order_release){ \
     asm volatile ( \
                  "        lwsync"); \
   } else if (__x__ == memory_order_seq_cst) {\
     asm volatile ( \
                  "        sync"); \
   }\
   *__p__ = __v__; \
   __v__; })


#define _ATOMIC_MODIFY_( __a__, __o__, __m__, __x__ ) \
({ volatile __typeof__((__a__)->__f__)* __p__ = &((__a__)->__f__); \
   __typeof__(__m__) __v__ = (__m__); \
   __typeof__(__m__) __t__; \
   int size = sizeof(__v__); \
   switch (size) { \
   	case 1: { \
        assert(false);\
   	} \
        break; \
   	case 2: { \
        assert(false);\
   	} \
        break; \
   	case 4: { \
	   if(__o__ == ops_swap) \
	     asm volatile ( \
		                          "        lwarx        %0,0,%2\n" \
					  "        stwcx.       %1,0,%2\n" \
					  "        bne-         $-8\n"  \
					  : "=&r" (__t__) \
					  : "r" (__v__), "r" (__p__) \
					  : "cc", "memory"); \
	   else if(__o__ == ops_add) { \
	     asm volatile ( \
		                          "        lwarx        %0,0,%2\n" \
					  "        add          %0,%1,%0\n" \
					  "        stwcx.       %0,0,%2\n" \
					  "        bne-         $-12\n"  \
					  : "=&r" (__t__) \
					  : "r" (__v__), "r" (__p__) \
					  : "cc", "memory"); \
} \
	   else if(__o__ == ops_sub) \
	     asm volatile ( \
		                          "        lwarx        %0,0,%2\n" \
					  "        subf         %0,%1,%0\n" \
					  "        stwcx.       %0,0,%2\n" \
					  "        bne-         $-12\n"  \
					  : "=&r" (__t__) \
					  : "r" (__v__), "r" (__p__) \
					  : "cc", "memory"); \
	   else if(__o__ == ops_and) \
	     asm volatile ( \
		                          "        lwarx        %0,0,%2\n" \
					  "        and          %0,%1,%0\n" \
					  "        stwcx.       %0,0,%2\n" \
					  "        bne-         $-12\n"  \
					  : "=&r" (__t__) \
					  : "r" (__v__), "r" (__p__) \
					  : "cc", "memory"); \
	   else if(__o__ == ops_or) \
	     asm volatile ( \
		                          "        lwarx        %0,0,%2\n" \
					  "        or           %0,%1,%0\n" \
					  "        stwcx.       %0,0,%2\n" \
					  "        bne-         $-12\n"  \
					  : "=&r" (__t__) \
					  : "r" (__v__), "r" (__p__) \
					  : "cc", "memory"); \
	   else if(__o__ == ops_xor) \
	     asm volatile ( \
		                          "        lwarx        %0,0,%2\n" \
					  "        xor          %0,%1,%0\n" \
					  "        stwcx.       %0,0,%2\n" \
					  "        bne-         $-12\n"  \
					  : "=&r" (__t__) \
					  : "r" (__v__), "r" (__p__) \
					  : "cc", "memory"); \
   	} \
        break; \
   	case 8: { \
	   if(__o__ == ops_swap) \
    	 asm volatile ( \
		                          "        ldarx        %0,0,%2\n" \
					  "        stdcx.       %1,0,%2\n" \
					  "        bne-         $-8\n"  \
					  : "=&r" (__t__) \
					  : "r" (__v__), "r" (__p__) \
					  : "cc", "memory"); \
	   else if(__o__ == ops_add) \
	     asm volatile ( \
		                          "        ldarx        %0,0,%2\n" \
					  "        add          %0,%1,%0\n" \
					  "        stdcx.       %0,0,%2\n" \
					  "        bne-         $-12\n"  \
					  : "=&r" (__t__) \
					  : "r" (__v__), "r" (__p__) \
					  : "cc", "memory"); \
	   else if(__o__ == ops_sub) \
	     asm volatile ( \
		                          "        ldarx        %0,0,%2\n" \
					  "        subf         %0,%1,%0\n" \
					  "        stdcx.       %0,0,%2\n" \
					  "        bne-         $-12\n"  \
					  : "=&r" (__t__) \
					  : "r" (__v__), "r" (__p__) \
					  : "cc", "memory"); \
	   else if(__o__ == ops_and) \
    	 asm volatile ( \
		                          "        ldarx        %0,0,%2\n" \
					  "        and          %0,%1,%0\n" \
					  "        stdcx.       %0,0,%2\n" \
					  "        bne-         $-12\n"  \
					  : "=&r" (__t__) \
					  : "r" (__v__), "r" (__p__) \
					  : "cc", "memory"); \
	   else if(__o__ == ops_or) \
    	 asm volatile ( \
		                          "        ldarx        %0,0,%2\n" \
					  "        or           %0,%1,%0\n" \
					  "        stdcx.       %0,0,%2\n" \
					  "        bne-         $-12\n"  \
					  : "=&r" (__t__) \
					  : "r" (__v__), "r" (__p__) \
					  : "cc", "memory"); \
	   else if(__o__ == ops_xor) \
    	 asm volatile ( \
	    	                          "        ldarx        %0,0,%2\n" \
					  "        xor          %0,%1,%0\n" \
					  "        stdcx.       %0,0,%2\n" \
					  "        bne-         $-12\n"  \
					  : "=&r" (__t__) \
					  : "r" (__v__), "r" (__p__) \
					  : "cc", "memory"); \
   	} \
        break; \
   } \
   if((__x__ == memory_order_seq_cst) || (__x__ = memory_order_acq_rel)) \
     asm volatile ( \
                  "        isync"); \
   __t__; })


#define _ATOMIC_CMPSWP_( __a__, __e__, __m__, __x__ ) \
({ volatile __typeof__((__a__)->__f__)* __p__ = &((__a__)->__f__); \
   __typeof__(__e__) __q__ = (__e__); \
   __typeof__(__m__) __v__ = (__m__); \
   bool __r__; \
   __typeof__(__m__) ret; \
   __typeof__(__m__) tmp = 0; \
   if((__x__ == memory_order_release) || (__x__ == memory_order_acq_rel)) \
     asm volatile ( \
                  "        lwsync"); \
   else if (__x__ == memory_order_seq_cst) \
     asm volatile ( \
                  "        sync"); \
   int size = sizeof(__v__); \
   switch (size) { \
   	case 1: { \
        assert(false);\
   	} \
        break; \
   	case 2: { \
        assert(false);\
   	} \
        break; \
   	case 4: { \
   asm volatile   ( \
                  "        lwarx        %0,0,%1\n" \
                  "        cmpw         %2,%0\n" \
                  "        bne-         $+12\n" \
                  "        stwcx.       %3,0,%1\n" \
                  "        bne-         $-16\n" \
		  : "=&r"(ret)\
		  : "r"(__p__), "r"(*__q__), "r"(__v__) \
		  : "cr0", "memory"); \
   	} \
        break; \
   	case 8: { \
   asm volatile   ( \
                  "        ldarx        %0,0,%1\n" \
                  "        cmpd         %2,%0\n" \
                  "        bne          $+12\n" \
                  "        stdcx.       %3,0,%1\n" \
                  "        bne-         $-16\n" \
		  : "=&r"(ret) \
		  : "r"(__p__), "r"(*__q__), "r"(__v__) \
		  : "cr0", "memory"); \
   	} \
        break; \
   } \
   if((__x__ == memory_order_seq_cst) || (__x__ == memory_order_acq_rel)) \
     asm volatile ( \
                  "        isync"); \
    if(ret == *__q__) { __r__ = true;} \
    else {*__q__ = ret;  __r__ = false;} \
   __r__; })


#define _ATOMIC_FENCE_( __a__, __x__ ) \
({ if(__x__ == memory_order_acquire) \
     asm volatile ( \
                  "        lwsync"); \
   else if(__x__ == memory_order_release) \
     asm volatile ( \
                  "        lwsync"); \
   else if(__x__ == memory_order_acq_rel) \
     asm volatile ( \
                  "        sync"); \
   else if(__x__ == memory_order_seq_cst) \
     asm volatile ( \
                  "        sync"); \
   })



/*************************************************************************************/


/* To facilitate optimal storage use, the proposal supplies a feature macro that indicates
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
