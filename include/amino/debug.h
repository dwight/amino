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
 *
 *  Change History:
 *
 *  yy-mm-dd  Developer  Defect     Description
 *  --------  ---------  ------     -----------
 *  08-08-03  ganzhi     N/A        Initial implementation
 */

/**
 * This file is used to store all kinds of utility function for debugging other
 * components
 */
#ifndef AMINO_DEBUG_H
#define AMINO_DEBUG_H
#include <amino/config.h>

#ifdef PPC
#include <sys/processor.h>
#endif

using namespace std;

inline void print_backtrace() {
}

/**
 * This function is used to prevent compiler optimization of performance test code such as pure read test
 */
inline void prevent_opt(void * address){
    char * c_addr = (char *)address;
    long tmp1;
#ifdef X86
    __asm__ __volatile__(
            "mov (%1), %0    \n\t"
            :"=r"(tmp1)
            :"r"(c_addr)
            );
#elif defined(PPC)
    __asm__ __volatile__(
            ""
            :"=r"(tmp1)
            :"r"(c_addr)
            );
#endif
}

/**
 * @brief this function will return a timestamp.
 *
 * it can be used to measure cycles used by several lines of code.
 *
 */
inline unsigned long cycleStamp(){
    long low;
    long high;
#ifdef X86
    __asm__ __volatile__(
            "XOR %0,%0;"
            "CPUID;"
            "RDTSC"
            :"=a" (low), "=d" (high)
            );
    return low;
#elif defined(PPC)
    unsigned long ret;
    __asm__ __volatile__("mftb %0" : "=r" (ret) : ); 
    return ret;
#endif
    return 0;
}

/**
 * @brief this function will return a timestamp.
 *
 * it can be used to measure cycles used by several lines of code.
 * it should be call at the beigining of measuring the cycles.
 */

inline unsigned long cycleStampBegin()
{
#ifdef PPC
    //First set the thread's cpu affinity. 
    long noProcessorOnline = sysconf(_SC_NPROCESSORS_ONLN);
    long noProcessorConf = sysconf(_SC_NPROCESSORS_CONF);
 
    if ( noProcessorConf <= 1) 
        return cycleStamp();

    int i = 0;
    int retValue = -1;
    int pid = getpid();

    //try to bind a processor    
    for ( ; i<noProcessorOnline; i++)
    {
       retValue = bindprocessor(BINDPROCESS, pid, i);   
       if (retValue == 0)
           break;
    }
    
    //Second get the begin cycle. 
    return  cycleStamp();
#endif
//TODO
#ifdef X86
    return 0;
#endif
}

//This function get the cycle at the end of measuring.
#define cycleStampEnd cycleStamp

#endif
