#!/bin/bash
cd "$(dirname "$0")"
gcc		-Wextra -Wall -std=c99 -s -O2 -c	tagha_api.c
ar			cr libtagha.a	tagha_api.o

#gcc		-Wextra -Wall -std=c99 -Os -S -masm=intel tagha_api.c

rm	tagha_api.o


g++			-Wextra -Wall -std=c++17 -O2 -c	tagha_api_cpp.cpp -L. -ltagha
ar			cr libtaghacpp.a	tagha_api_cpp.o

rm	tagha_api_cpp.o


#clang-3.5			-Wextra -Wall -std=c99 -O2 -funroll-loops -finline-functions -fno-math-errno -fexpensive-optimizations -c	tagha_api.c
#ar			cr libtaghaclang.a	tagha_api.o
#rm	tagha_api.o

#clang-3.5 -Os -S -emit-llvm tagha_api.c