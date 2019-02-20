@echo off

set CC=clang-cl
set CFLAGS=-Wextra -Wall -Xclang -std=c99 -O2 
set TESTFLAGS=-Wextra -Wall -Xclang -std=c99 -g -O2

set SRCS=tagha_api.c

set OBJS=%SRCS:.c=.obj%
set LIBNAME=libtagha

set ACTION=%1

if [%1] == [] set ACTION=tagha
goto %ACTION%

:tagha
       %CC% %CFLAGS% -c libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/mempool.c libharbol/linkmap.c %SRCS%
       llvm-ar cr %LIBNAME%.a stringobj.obj vector.obj hashmap.obj mempool.obj linkmap.obj %OBJS%
       %CC% /LD stringobj.obj vector.obj hashmap.obj mempool.obj linkmap.obj %OBJS% -o %LIBNAME%.lib
       goto END

:tagha_asm
       %CC% %CFLAGS% libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/bytebuffer.c libharbol/linkmap.c tagha_assembler/tagha_assembler.c -o tagha_asm.exe
       goto END

:tagha_libc
       %CC% %CFLAGS% -c libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/mempool.c libharbol/linkmap.c tagha_libc/tagha_ctype.c tagha_libc/tagha_stdio.c tagha_libc/tagha_stdlib.c tagha_libc/tagha_string.c tagha_libc/tagha_time.c tagha_libc/tagha_module.c
       llvm-ar cr libtagha_libc.a stringobj.obj vector.obj hashmap.obj mempool.obj linkmap.obj tagha_ctype.obj tagha_stdio.obj tagha_stdlib.obj tagha_string.obj  tagha_time.obj tagha_module.obj
       %CC% /LD %LIBNAME%.lib stringobj.obj vector.obj hashmap.obj mempool.obj linkmap.obj tagha_ctype.obj tagha_stdio.obj tagha_stdlib.obj tagha_string.obj tagha_time.obj tagha_module.obj -o libtagha_libc.lib
       goto END

:testapp
       %CC% %CFLAGS% test_hostapp.c -L. -ltagha -ltagha_libc -o tagha_testapp /link msvcrt.lib 
       goto END

:clean
       del *.obj
       goto END

:END
echo %ACTION% is done