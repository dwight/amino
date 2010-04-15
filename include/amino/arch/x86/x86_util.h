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

#ifndef AMINO_UTIL_X86_H
#define AMINO_UTIL_X86_H

const int CACHE_PER_CORE=512000;
const int CACHE_LINE_SIZE=64;

#define LOCK "lock;"

//fail: todo check this for WIN32
//optimization barrier
#if defined(_WIN32)
#define compiler_barrier() _ReadWriteBarrier()
#else
#define compiler_barrier()  __asm__ __volatile__ ("":::"memory")
#endif

/**
 * According to David Dice's understanding,
 * http://blogs.sun.com/dave/entry/java_memory_model_concerns_on,
 * recent x86 manuals have relaxed the hardware memory model. 
 * mfence doesn't get sequential consistency any more.
 * So here we discriminate CC fence from SC fence...
 */

//fence between load and load, store and store, load and store, store and load
#define LLFENCE __asm__ __volatile__ ("lfence":::"memory")
#define SSFENCE __asm__ __volatile__ ("sfence":::"memory")
#define LSFENCE __asm__ __volatile__ ("mfence":::"memory")
#define SLFENCE __asm__ __volatile__ ("mfence":::"memory")

#define CC_FENCE __asm__ __volatile__ ("mfence":::"memory")

#if defined(BIT32)
#define SC_FENCE __asm__ __volatile__ ("mfence":::"memory")
#elif defined(BIT64)
#define SC_FENCE __asm__ __volatile__ (LOCK "addl $0, 0(%%rsp)":::"memory")
#endif


inline int getProcessNum(){
    //TODO add code to get this information
    return 8;
}

/**
 * @brief Do CAS operation on address specified by parameter
 * <code>address</code>
 *
 * CAS is the abbrev. for <i>Compare and Swap</i> or <i>Compare and Set</i>, which
 * is a very important instruction for implementing lock-free algorithm. This
 * instruction can do following steps atomically: 
 *
 * <ol>
 * <li>Read the value referenced by a shared pointer</li>
 * <li>Compare the value with a older value</li>
 * <li>If equal, put the new value to the address specified by the shared pointer</li>
 * <li>Return</li>
 * </ol>
 *
 * @param address The memory address to be updated atomicially
 * @param oldV_a A pointer to the old value. Only if the value referenced by
 * address equals to the old value, CompareAndSet operation can succeed.
 * @param newV The new value which will be put to address when CAS succeeds
 * @param size The number of bytes that cas should handle. It can be 1,2, and 4
 * on 32-bit platform and 1,2,4, and 8 on 64-bit platform.
 *
 * @return true, if a compareAndSet operation succeeds; else return false. 
 *
 */
inline bool cas(volatile void* address, void* oldV_a, unsigned long newV, int size)
{
#if defined(_WIN32)
cout << "FAIL1" << endl;
return false;
#else
    //make sure address alignment
    bool ret;
    switch(size){
        case 1:
            {
                unsigned char* addr = (unsigned char*)address;
                unsigned char* oldV_addr = (unsigned char*)oldV_a;
                __asm__ __volatile__(
                        LOCK "cmpxchgb %b3, %2; setz %0;\n"
                        : "=q"(ret), "=a"(*(oldV_addr)), "=m"(*(addr))
                        : "c" (newV), "1"(*(oldV_addr))
                        : "memory", "cc");
                return ret;
            }
        case 2:
            {
                unsigned short* addr = (unsigned short*)address;
                unsigned short* oldV_addr = (unsigned short*)oldV_a;
                __asm__ __volatile__(
                        LOCK "cmpxchgw %w3, %2; setz %0;\n"
                        : "=q"(ret), "=a"(*(oldV_addr)), "=m"(*(addr))
                        : "r" (newV), "1"(*(oldV_addr))
                        : "memory", "cc");
                return ret;
            }
#if defined(BIT32)
        case 4:
            {
                unsigned long* addr = (unsigned long*)address;
                unsigned long* oldV_addr = (unsigned long*)oldV_a;
                __asm__ __volatile__(
                        LOCK "cmpxchgl %3, %2; setz %0;\n"
                        : "=q"(ret), "=a"(*(oldV_addr)), "=m"(*(addr))
                        : "r" (newV), "1"(*(oldV_addr))
                        : "memory", "cc");
                return ret;
            }
#elif defined(BIT64)
        case 4:
            {
                unsigned int* addr = (unsigned int*)address;
                unsigned int* oldV_addr = (unsigned int*)oldV_a;
                __asm__ __volatile__(
                        LOCK "cmpxchgl %k3, %2; setz %0;\n"
                        : "=q"(ret), "=a"(*(oldV_addr)), "=m"(*(addr))
                        : "r" (newV), "1"(*(oldV_addr))
                        : "memory", "cc");
                return ret;
            }
        case 8:
            {
                unsigned long* addr = (unsigned long*)address;
                unsigned long* oldV_addr = (unsigned long*)oldV_a;
                __asm__ __volatile__(
                        LOCK "cmpxchgq %3, %2; setz %0;\n"
                        : "=q"(ret), "=a"(*(oldV_addr)), "=m"(*(addr))
                        : "r" (newV), "1"(*(oldV_addr))
                        : "memory", "cc");
                return ret;
            }
#endif
    } //end switch
    return false;
#endif
}

/**
 * @brief This function is used to do CAS to two consecutive machine words.
 *
 * Since normal cas() can at most apply to one machine word which has as many
 * bits as address line. This function aims to provide CAS operation on double
 * width data. Eg, for 32bit platform, this function can operate 64bit data;
 * for 64bit platform, this function can operate 128bit data.
 *
 * @param ptr The memory address to be updated atomicially
 * @param old_addr A pointer to the old value. Only if the value referenced by
 * address equals to the old value, CompareAndSet operation can succeed.
 * @param new_v The new value which will be put to address when CAS succeeds
 *
 * @return true, if a compareAndSet operation succeeds; else return false. 
 */
inline bool casd(volatile void *ptr, void *old_addr, const void * new_v) {
#if defined(_WIN32)
cout << "fail2" << endl;
return false;
#else
    unsigned long * old_addr_long = (unsigned long *) old_addr;
    const unsigned long * new_v_long = (const unsigned long *) new_v;
    bool ret;
#if defined(BIT64)
    __asm__ __volatile__(
            "lock\n\t"
            "cmpxchg16b %5\n\t"
            "setz %0\n"
            : "=b"(ret), "=d"(old_addr_long[1]), "=a"(old_addr_long[0]) /*Output*/
            : "0"(new_v_long[0]), /*Input*/
            "c"(new_v_long[1]),
            "m"(*(long *)ptr),
            "1"(old_addr_long[1]),
            "2"(old_addr_long[0])
            : "memory", "cc"); /*Overwrites*/
#else
#ifdef __PIC__
    /**
     * In -fPIC code, we can't directly use regiater ebx since it's used for
     * "Global Offset Table". So we save ebx in stack at first and restore it
     * when finished
     */
    __asm__ __volatile__(
            "push   %%ebx     \n\t"
            "movl  %3,  %%ebx   \n\t"
            "lock   \n\t"
            "cmpxchg8b  %5       \n\t"
            "setz   %0 \n\t"
            "popl   %%ebx   \n\t"
            : "=q"(ret), "=d"(old_addr_long[1]), "=a"(old_addr_long[0]) /*Output*/
            : "m"(new_v_long[0]), /*Input*/
            "c"(new_v_long[1]),
            "m"(*((long*)(ptr))),
            "1"(old_addr_long[1]),
            "2"(old_addr_long[0])
            : "memory", "cc");
#else
    __asm__ __volatile__(
            "lock   \n\t"
            "cmpxchg8b  %5       \n\t"
            "setz   %0 \n\t"
            : "=b"(ret), "=d"(old_addr_long[1]), "=a"(old_addr_long[0]) /*Output*/
            : "0"(new_v_long[0]), /*Input*/
            "c"(new_v_long[1]),
            "m"(*((long*)(ptr))),
            "1"(old_addr_long[1]),
            "2"(old_addr_long[0])
            : "memory", "cc");
#endif //__PIC__
#endif //BIT64
    return ret;
#endif
}

/**
 * @brief atomically exchange content of two variables
 *
 * For "swap" operation. xchg always implies a full memory barrier on X86, even
 * without LOCK prefix. It essentially stores the value v into ptr, and return
 * the value in ptr before the store operation. I am not quite sure return
 * "unsigned long" will meet the demand, also for qualifiers of "v"
 */
inline unsigned long xchg(volatile void * ptr, unsigned long v, int size)
{
#if defined(_WIN32)
cout << "fail3" << endl;
#else
    switch (size) {
        case 1:
            {
                unsigned char* addr = (unsigned char*)ptr;
                __asm__ __volatile__("xchgb %b0,%1"
                        :"=q" (v)                /* cannot be r ? */
                        :"m" (*addr), "0" (v)
                        :"memory");
                break;
            }
        case 2:
            {
                unsigned short* addr = (unsigned short*)ptr;
                __asm__ __volatile__("xchgw %w0,%1"
                        :"=r" (v)
                        :"m" (*addr), "0" (v)
                        :"memory");
                break;
            }
#if defined(__i386__)
        case 4:
            {
                unsigned long* addr = (unsigned long*)ptr;
                __asm__ __volatile__("xchgl %0,%1"
                        :"=r" (v)
                        :"m" (*addr), "0" (v)
                        :"memory");
                break;
            }
#elif defined(__x86_64__)
        case 4:
            {
                unsigned int* addr = (unsigned int*)ptr;
                __asm__ __volatile__("xchgl %k0,%1"
                        :"=r" (v)
                        :"m" (*addr), "0" (v)
                        :"memory");
                break;
            }
        case 8:
            {
                unsigned long* addr = (unsigned long*)ptr;
                __asm__ __volatile__("xchgq %0,%1"
                        :"=r" (v)
                        :"m" (*addr), "0" (v)
                        :"memory");
                break;
            }
#endif
    }
    return v; //the value stored in *ptr before this xchg
#endif
}
#endif
