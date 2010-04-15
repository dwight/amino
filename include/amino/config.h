#include <amino/platform.h>


#ifdef PPC32_AIX_XLC
#define PPC
#define BIT32
#define AIX
#define XLC
#endif

#ifdef PPC32_LINUX_GCC
#define PPC
#define BIT32
#define LINUX
#define GCC
#endif

#ifdef PPC64_AIX_XLC
#define PPC
#define BIT64
#define AIX
#define XLC
#endif

#ifdef PPC64_AIX_GCC
#define PPC
#define BIT64
#define AIX
#define GCC
#endif

#ifdef PPC64_LINUX_GCC
#define PPC
#define BIT64
#define LINUX
#define GCC
#endif

#ifdef AMD64_LINUX_GCC
#define X86
#define BIT64
#define LINUX
#define GCC

//if there is wider cas than length of machine word, such as CMPXCHG8/16
#define CASD
#endif

#ifdef IA32_LINUX_GCC
#define X86
#define BIT32
#define LINUX
#define GCC

#define CASD
#endif

#ifdef SPARC32_SOLARIS_SCC
#define SPARC
#define BIT32
#define SOLARIS
#define SCC
#endif

