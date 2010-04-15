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

const int CACHE_PER_CORE=512000;
const int CACHE_LINE_SIZE=64;


//optimization barrier
#define compiler_barrier()  __asm__ __volatile__ ("":::"memory")

#define LFENCE __asm__ __volatile__ ("":::"memory")
#define SFENCE __asm__ __volatile__ ("":::"memory")
#define CC_FENCE __asm__ __volatile__ ("":::"memory")

#if defined(BIT32)
#define SC_FENCE __asm__ __volatile__ ("":::"memory")
#elif defined(BIT64)
#define SC_FENCE __asm__ __volatile__ ("":::"memory")
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
#if defined(BIT32)
#elif defined(BIT64)
#endif
    return false;
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
    return false;
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
    return v; //the value stored in *ptr before this xchg
}
