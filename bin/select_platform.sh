#!/bin/bash

PS3="Which processor are you using? "
select processor in "IA32" "AMD64" "PPC32" "PPC64"
do
    if [ "$processor" == "" ]
    then
        echo -e "Invalid entry.\n"
        continue
    elif [ "$processor" == "IA32" ]
    then
        make_processor="ia32"
        break
    elif [ "$processor" == "AMD64" ]
    then
        make_processor="amd64"
        break
    elif [ "$processor" == "PPC32" ]
    then
        make_processor="ppc"
        break
    elif [ "$processor" == "PPC64" ]
    then
        make_processor="ppc64"
        break
    fi
done

PS3="Which operating system are you using? "
select os in "LINUX" "AIX"
do
    if [ "$os" == "" ]
    then
        echo -e "Invalid entry.\n"
        continue
    elif [ "$os" == "LINUX" ]
    then
        make_os="linux"
        break
    elif [ "$os" == "AIX" ]
    then
        make_os="aix"
        break
    fi
done

PS3="Which compiler are you using? "
select option in "GNU C/C++ Compiler" "IBM XLC" 
do
    if [ "$option" == "" ]
    then
        echo -e "Invalid entry.\n"
        continue
    elif [ "$option" == "GNU C/C++ Compiler" ]
    then
        compiler="GCC"
        make_compiler="gcc"
        break
    elif [ "$option" == "IBM XLC" ]
    then
        compiler="XLC"
        make_compiler="vacpp"
        break
    fi
done


# The sole purpose of this file is containing platform macro, which can be
# chosen from one of following flags:
#
#      1.  PPC32_AIX_XLC
#      2.  PPC64_AIX_XLC
#      3.  PPC64_AIX_GCC
#      4.  PPC32_LINUX_GCC
#      5.  PPC64_LINUX_GCC
#      6.  AMD64_LINUX_GCC
#      7.  IA32_LINUX_GCC

echo "#define ""$processor"_"$os"_"$compiler" > ../include/amino/platform.h
echo "PLATFORM=""$make_processor"-"$make_os"-"$make_compiler" > makeOpts.mk
