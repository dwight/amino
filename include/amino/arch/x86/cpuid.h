#ifndef AMINO_CPUID_H
#define AMINO_CPUID_H

inline void cpuid(int id, int * result){
#if defined(BIT32)
    __asm__(
            "PUSHL %%ebx\n\t"
            "MOVL  %1, %%ebx\n\t"
            "CPUID\n\t"
            "MOVL  %%ebx, %1\n\t"
            "MFENCE\n\t"
            "POPL  %%ebx\n\t"
           :"=a"(result[0]),"=m"(result[1]),"=c"(result[2]),"=d"(result[3])
           :"0"(id)
           : 
            );
#else
    __asm__(
            "CPUID\n\t"
           :"=a"(result[0]),"=b"(result[1]),"=c"(result[2]),"=d"(result[3])
           :"0"(id)
           : 
            );
#endif
}

#endif
