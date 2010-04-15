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

#ifndef VECTOR_INSTR_H
#define VECTOR_INSTR_H

#ifdef DEBUG
#include <assert.h>
#include <iostream>
using namespace std;
#endif

#ifdef __cplusplus
extern "C"{
#endif

/**
 * Compare elements of each vector one by one. And move the
 * larger element to arg2, the smaller element to arg1.
 *
 * @param arg1 An int vector with a length of 4
 * @param arg2 An int vector with a length of 4
 * @return true if swap happened at any elements
 */
#if 0
	//this version only works for x86 platform which supports SSE4
inline void vector_cmpswap_noret(int * arg1, int * arg2){
    __asm__(
            //copy vector arg1&arg2 to SIMD register
            "movdqa     (%0), %%xmm0\n\t"
            "movdqa     (%1), %%xmm1\n\t"

            //use pmax and pmin
            "pmaxsw     (%1), %%xmm0\n\t"
            "pminsw     (%0), %%xmm1\n\t"

            "movdqa     %%xmm0, (%1)\n\t"
            "movdqa     %%xmm1, (%0)\n\t"
            :
            :"r"(arg1), "r"(arg2)
            :"memory", "cc","%eax"
           );
}
#endif

/**
 * Compare elements of each vector one by one. And move the
 * larger element to arg2, the smaller element to arg1.
 *
 * @param arg1 An int vector with a length of 4
 * @param arg2 An int vector with a length of 4
 * @return true if swap happened at any elements
 */
inline void vector_cmpswap_noret(int * arg1, int * arg2){
    __asm__(
            //copy vector arg1&arg2 to SIMD register
            "movdqa     (%0), %%xmm0\n\t"
            "movdqa     (%1), %%xmm1\n\t"

            //make a copy
            "movdqa     %%xmm0, %%xmm2\n\t"

            //compare vector arg1 and arg2
            "pcmpgtd    %%xmm1, %%xmm0\n\t"
            "pmovmskb   %%xmm0, %%eax\n\t"
            "testw      %%ax,   %%ax\n\t"
            "jz         0f\n\t"
            "movdqa     %%xmm0, %%xmm4\n\t"

            "movdqa     %%xmm1, %%xmm3\n\t"
            "pand       %%xmm0, %%xmm1\n\t"
            "pandn      %%xmm2, %%xmm0\n\t"
            "pxor       %%xmm0, %%xmm1\n\t"
            "movdqa     %%xmm1, (%0)\n\t"

            "pand       %%xmm4, %%xmm2\n\t"
            "pandn      %%xmm3, %%xmm4\n\t"
            "pxor       %%xmm2, %%xmm4\n\t"
            "movdqa     %%xmm4, (%1)\n\t"
            "0:\n\t"
            :
            :"r"(arg1), "r"(arg2)
            :"memory", "cc","%eax"
           );
}

inline int vector_cmpswap(int * arg1, int * arg2){
#if defined(DEBUG) && defined (VERBOSE)
        assert(((unsigned long) arg1)%16 == 0);
        assert(((unsigned long) arg2)%16 == 0);
        cout<<"VECTOR_CMPSWAP{\n";
        printVector(arg1);
        printVector(arg2);
#endif
        char res;
        int mask=0;
        __asm__(
            //copy vector arg1 to SIMD register
            "movdqa     (%2), %%xmm0\n\t"
            "movdqa     %%xmm0, %%xmm2\n\t"

            //copy vector arg2 to SIMD register
            "movdqa     (%3), %%xmm1\n\t"
            "movdqa     %%xmm1, %%xmm3\n\t"

            //compare vector arg1 and arg2
            "pcmpgtd    %%xmm1, %%xmm0\n\t"
            //set return variable to true if cmp is not 0
            "xorb       %0, %0\n\t"
            "pmovmskb   %%xmm0, %1\n\t"
            "test       %1, %1\n\t"
            "jz         1f\n\t"
            "movb       $1, %0\n\t"

            //backup compare result
            "movdqa     %%xmm0, %%xmm4\n\t"

            "pand       %%xmm0, %%xmm1\n\t"
            "pandn      %%xmm2, %%xmm0\n\t"
            "pxor       %%xmm0, %%xmm1\n\t"
            "movdqa     %%xmm1, (%2)\n\t"

            "pand       %%xmm4, %%xmm2\n\t"
            "pandn      %%xmm3, %%xmm4\n\t"
            "pxor       %%xmm2, %%xmm4\n\t"
            "movdqa     %%xmm4, (%3)\n\t"
            "1:"
            :"=a"(res),"=d"(mask)
            :"S"(arg1), "D"(arg2)
            :"memory", "cc"
              );
#if defined(DEBUG) && defined (VERBOSE)
        printVector(arg1);
        printVector(arg2);
        for(int i=0;i<4;i++){
            if(arg1[i]>arg2[i]){
                cout<<"Error";
                assert(arg1[i]<=arg2[i]);
            }
        }
        cout <<"}END_VECTOR_CMPSWP\n";
#endif
        return res;
    }

/**
 * Compare elements of two vectors one by one. The method compares
 * element 1,2,3 of arg1 with element 2,3,4 of arg2. Element will
 * exchange position if element of arg1 is larger than element of
 * arg2
 *
 * @param arg1 An int vector with a length of 4
 * @param arg2 An int vector with a length of 4
 * @return true if swap happened at any elements
 */
inline int vector_cmpswap_skew(int * arg1, int * arg2){
    int tail1=arg1[3];

    arg1[3]=arg1[2];
    arg1[2]=arg1[1];
    arg1[1]=arg1[0];

    // this is required so that the first element won't
    // cause exchange
    arg1[0]=arg2[0];

    int res = vector_cmpswap(arg1, arg2);

    arg1[0]=arg1[1];
    arg1[1]=arg1[2];
    arg1[2]=arg1[3];
    arg1[3]=tail1;

    return res;
}

inline void vector_cmpswap_skew_noret(int * arg1, int * arg2){
    int tail1=arg1[3];

    arg1[3]=arg1[2];
    arg1[2]=arg1[1];
    arg1[1]=arg1[0];

    // this is required so that the first element won't
    // cause exchange
    arg1[0]=arg2[0];

    vector_cmpswap_noret(arg1, arg2);

    arg1[0]=arg1[1];
    arg1[1]=arg1[2];
    arg1[2]=arg1[3];
    arg1[3]=tail1;
}

/**
 * This function is used to transpose a 4X4 matrix of int
 */
inline void transpose4_4(int * array){
    __asm__(
            "movdqa    (%0), %%xmm0\n\t"
            "movdqa    %%xmm0, %%xmm4\n\t"
            "movdqa    16(%0), %%xmm1\n\t"
            "movdqa    %%xmm1, %%xmm5\n\t"
            "movdqa    32(%0), %%xmm2\n\t"
            "movdqa    48(%0), %%xmm3\n\t"
            "punpckhdq  %%xmm2, %%xmm0\n\t"
            "punpckldq  %%xmm2, %%xmm4\n\t"
            "punpckhdq  %%xmm3, %%xmm1\n\t"
            "punpckldq  %%xmm3, %%xmm5\n\t"

            "movdqa     %%xmm0, %%xmm2\n\t"
            "movdqa     %%xmm4, %%xmm3\n\t"

            "punpckhdq  %%xmm1, %%xmm0\n\t"
            "punpckldq  %%xmm1, %%xmm2\n\t"
            "punpckhdq  %%xmm5, %%xmm4\n\t"
            "punpckldq  %%xmm5, %%xmm3\n\t"

            "movdqa     %%xmm3, (%0)\n\t"
            "movdqa     %%xmm4, 16(%0)\n\t"
            "movdqa     %%xmm2, 32(%0)\n\t"
            "movdqa     %%xmm0, 48(%0)\n\t"
           :
           :"r"(array)
           :"memory", "cc"
            );
}

/**
 * Sort the vector array so that for each vector,
 * we have V[0]<V[1]<V[2]<V[3] for each vector V in the vector array.
 * Be careful, each 4x4 matrix is transposed.
 */
inline void sortInVec(int * array, int vec_len){
    int group_len = vec_len/4;
    int i,j,k;

    // Sort 4x4 matrix M in each loop
    for(i=0;i<group_len;i++){
        int * base = array+i*4*4;

        //let 1st row contains minimal elements of each column
        vector_cmpswap_noret(base, base+4);
        vector_cmpswap_noret(base, base+8);
        vector_cmpswap_noret(base, base+12);

        //let 2nd row contains 2nd minimal elements of each column
        vector_cmpswap_noret(base+4, base+8);
        vector_cmpswap_noret(base+4, base+12);

        //let 2nd row contains 2nd minimal elements of each column
        vector_cmpswap_noret(base+8, base+12);

        //transpose the matrix
        transpose4_4(base);
    }

    for(k=group_len*4;k<vec_len;k++){
        for(i=0;i<3;i++){
            for(j=i+1;j<4;j++){
                if(array[4*k+i]>array[4*k+j]){
                    int tmp = array[4*k+j];
                    array[4*k+j] = array[4*k+i];
                    array[4*k+i] = tmp;
                }
            }
        }
    }
}

inline void copyVector(int * arg1, int * arg2){
    __asm__(
            //copy vector arg1 to SIMD register
            "movdqa     (%0), %%xmm0\n\t"
            //copy register to memory
            "movdqa     %%xmm0, (%1)\n\t"
            :
            :"r"(arg1), "r"(arg2)
            :"memory", "cc"
           );
}

inline void swapVector(int * arg1, int * arg2){
    __asm__(
            //copy vector arg1 to SIMD register
            "movdqa     (%0), %%xmm0\n\t"
            "movdqa     (%1), %%xmm1\n\t"


            //copy register to memory
            "movdqa     %%xmm0, (%1)\n\t"
            "movdqa     %%xmm1, (%0)\n\t"

            :
            :"r"(arg1), "r"(arg2)
            :"memory", "cc"
           );
}

/**
 * @brief merge sort two length-4 int array with vector instructions
 *
 */
inline void vector_merge(int * arg1, int * arg2){
    vector_cmpswap_noret(arg1, arg2);

    int tail = arg2[3];
    arg2[3]=arg2[2];
    arg2[2]=arg2[1];
    arg2[1]=arg2[0];
    arg2[0]=arg1[0];

    vector_cmpswap_noret(arg1, arg2);
    arg2[0]=arg2[1];
    arg2[1]=arg2[2];
    arg2[2]=arg2[3];
    arg2[3]=tail;


    if(arg2[0]<arg1[2]){
        int tmp = arg1[2];
        arg1[2] = arg2[0];
        arg2[0] = tmp;
    }
    if(arg2[1]<arg1[3]){
        int tmp = arg1[3];
        arg1[3] = arg2[1];
        arg2[1] = tmp;
    }
    if(arg2[0]<arg1[3]){
        int tmp = arg1[3];
        arg1[3] = arg2[0];
        arg2[0] = tmp;
    }
}
#ifdef __cplusplus
}
#endif
#endif
