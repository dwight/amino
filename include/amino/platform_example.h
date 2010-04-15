/**
 * The sole purpose of this file is containing platform macro, which can be
 * chosen from one of following flags:
 *
 *      1.  PPC32_AIX_XLC
 *      2.  PPC64_AIX_XLC
 *      3.  PPC64_AIX_GCC
 *      4.  PPC32_LINUX_GCC
 *      5.  PPC64_LINUX_GCC
 *      6.  AMD64_LINUX_GCC
 *      7.  IA32_LINUX_GCC
 *
 * Bash script bin/select_platform.sh will generate this file automatically or
 * you could define it manually.
 *
 */

// #define PPC64_AIX_GCC
