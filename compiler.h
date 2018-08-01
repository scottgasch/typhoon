/**

Copyright (c) Scott Gasch

Module Name:

    system.h

Abstract:

Author:

    Scott Gasch (scott@wannabe.guru.org) 8 Apr 2004

Revision History:

    $Id: compiler.h 349 2008-06-28 02:48:59Z scott $

**/

#ifndef COMPILER_H_
#define COMPILER_H_

#if defined _MSC_VER

#include <io.h>

#pragma warning(disable:4201) // nonstandard extension: nameless struct/union
#pragma warning(disable:4214) // nonstandard extension: bitfields struct

#define PROFILE                    ""
#define ALIGN64                    __declspec(align(64))
#define TB_FASTCALL                __fastcall
#define COMPILER_STRING            "Microsoft(R) Visual C/C++ %u\n", _MSC_VER
#define open                       _open
#define write                      _write
#define COMPILER_LONGLONG          __int64
#define COMPILER_VSNPRINTF         _vsnprintf
#define CDECL                      __cdecl
#define FASTCALL                   __fastcall
#define INLINE                     __inline
#define FORCEINLINE                __forceinline
#define NORETURN
#define CONSTFUNCT
#define COMPILER_LONGLONG_HEX_FORMAT "I64x"
#define COMPILER_LONGLONG_UNSIGNED_FORMAT "I64u"
#define STRNCMPI                   _strnicmp
#define STRCMPI                    strcmpi
#define STRDUP                     SystemStrDup

#elif defined __GNUC__

#include <unistd.h>

extern int errno;

#define COMPILER_STRING            "GCC "__VERSION__"\n"
#define BUILD_STRING               __OPTIMIZE__

#define O_BINARY                   (0)
#define O_RANDOM                   (0)
#define _S_IREAD                   (0)
#define _S_IWRITE                  (0)

#define ALIGN64 // __attribute__ (aligned(64)) does not work
#define COMPILER_LONGLONG          long long
#define COMPILER_VSNPRINTF         vsnprintf
#ifndef SIXTYFOUR
#define CDECL                      __attribute__((cdecl))
#endif
#define FASTCALL                   __attribute__((__regparm__(3)))
#define INLINE                     __inline__
#define FORCEINLINE                __attribute__((always_inline))
#define NORETURN                   __attribute__((noreturn))
#define CONSTFUNCT                 __attribute__((const))
#define COMPILER_LONGLONG_HEX_FORMAT "llx"
#define COMPILER_LONGLONG_UNSIGNED_FORMAT "llu"
#define STRNCMPI                   strncasecmp
#define STRCMPI                    strcasecmp
#define STRDUP                     SystemStrDup
#define TB_FASTCALL // __attribute__((__regparm__(3)))

#else

#error "Unknown compiler, please edit compiler.h"

#endif /* What compiler?! */

#endif /* COMPILER_H_ */
