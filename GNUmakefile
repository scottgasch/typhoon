# Makefile (for typhoon/GNU-style make).  Recognized gmake variables:
#
#  DEBUG=1: make debug version (includes asserts and sanity checks)
#  TEST=1: include self-test code in the binary produced
#  ASM=1: create assembly code, does not link final binary
#  EVAL_DUMP=1: make a version that can dump eval breakdowns
#  EVAL_HASH=1: hash eval scores
#  EVAL_TIME=1: make a version that counts cycles spent in eval
#  PERF_COUNTERS=1: make version with perf counters enabled
#  BOUNDS_CHECKING=1: make version with bounds checking enabled
#  MP=1: make version for multi-processor machine
#  DUMP_TREE=1: internal tree dumping version
#  SEE_HEAPS=1: use minheaps in the SEE code
#  OSX=1: build for an Intel-based Apple Mac (omit this for FreeBSD/Linux)
#  USE_READLINE=1: link against the GNU readline library
#  SIXTYFOUR=1: make an X64 binary
#  CROUTINES=1: use the C versions of the asm routines [slower]
#  EVERYTHING=1: everything everything everything everything
#
# $Id$
#
CC 		= 	clang
CXX		=	clang++
NASM		=	yasm
ifdef OSX
 LIBRARIES	=	-pthread -lc
 ifdef SIXTYFOUR
  $(warning you will need to use yasm for now to get x64 on OSX)
  NASMFLAGS	=	-f macho64 -dOSX -d_X64_
 else
  NASMFLAGS	=	-f macho -dOSX
 endif
else
 LIBRARIES	=	-pthread -lc -lstdc++ 
 ifdef SIXTYFOUR
  NASMFLAGS	=	-f elf64 -d__GNUC__
 else
  NASMFLAGS	=	-f elf -d__GNUC__
 endif
endif
RM		= 	/bin/rm

#
# Setup commandline based on make defines
#
ifdef ASM
 ifdef SIXTYFOUR
  PROFILE	=	-S -fverbose-asm -D_X64_
 else
  PROFILE	=	-S -fverbose-asm -D_X86_
 endif
else
 ifdef SIXTYFOUR
  PROFILE 	=	-D_X64_
 else
  PROFILE 	=	-D_X86_
 endif
endif

ifdef OSX
 PROFILE	+=	-DOSX
endif

ifdef GENETIC
 PROFILE	+=	-DGENETIC
endif

ifdef DEBUG
 PROFILE 	+=	-g -O2 -DDEBUG
 BINARY 	=	_typhoon
else
 PROFILE	+=	-g -O3 -fexpensive-optimizations -ffast-math -finline-functions
 BINARY 	=	typhoon
endif

ifdef USE_READLINE
 LIBRARIES	+=	-lreadline
 PROFILE	+=	-DUSE_READLINE
endif

ifdef SIXTYFOUR
 PROFILE	+=	-m64
 NASMFLAGS	+=	-d_X64_
else
 PROFILE	+=	-m32
 NASMFLAGS	+=	-d_X86_
endif

ifdef CROUTINES
 PROFILE	+=	-DCROUTINES
endif

ifdef EVERYTHING
PROFILE 	+=	-DEVAL_DUMP -DEVAL_HASH -DEVAL_TIME -DPERF_COUNTERS -DMP -DSMP -DTEST_NULL -DDUMP_TREE -fbounds-checking 
else
ifdef EVAL_DUMP
PROFILE		+= 	-DEVAL_DUMP
endif
ifdef EVAL_HASH
PROFILE 	+= 	-DEVAL_HASH
endif
ifdef EVAL_TIME
PROFILE		+= 	-DEVAL_TIME
endif
ifdef PERF_COUNTERS
PROFILE		+=	-DPERF_COUNTERS
endif
ifdef BOUNDS_CHECKING
PROFILE		+=	-fbounds-checking
endif
ifdef MP
PROFILE		+=	-DMP -DSMP
endif
ifdef TEST_NULL
PROFILE		+=	-DTEST_NULL
endif
ifdef DUMP_TREE
PROFILE 	+=  -DDUMP_TREE
endif
endif # EVERYTHING

CFLAGS		=	-DPROFILE="\"$(PROFILE)\"" $(PROFILE) -Wall
HEADERS 	=	chess.h compiler.h

#
# ---> .o, not .c! <---
# 
OBJS    =       main.o root.o search.o searchsup.o draw.o dynamic.o \
		hash.o eval.o evalhash.o x86.o pawnhash.o bitboard.o \
		generate.o see.o move.o movesup.o command.o script.o \
		input.o vars.o util.o unix.o gamelist.o mersenne.o \
		sig.o piece.o ics.o san.o fen.o book.o bench.o board.o \
		data.o probe.o egtb.o recogn.o poshash.o list.o x64.o

ifdef TEST
# ---> .o, not .c! <---
# 
OBJS    +=      testdraw.o testmove.o testfen.o testgenerate.o \
		testsan.o testics.o testeval.o testbitboard.o \
		testhash.o testsee.o testsearch.o testsup.o
PROFILE +=	-DTEST
else
ifdef EVAL_DUMP
OBJS	+=	testeval.o testsup.o
endif
endif
ifdef DUMP_TREE
OBJS	+=	dumptree.o
endif
ifdef MP
OBJS	+=	split.o
endif

.SUFFIXES:	.c .o
.SUFFIXES:	.cpp .o
.SUFFIXES:	.asm .o

.PHONY:		all
all:		$(BINARY)

$(BINARY):	$(OBJS)
		$(CXX) $(PROFILE) -o $(BINARY) -Wall $(LIBRARIES) \
		$(OBJS)

%.o:        %.c $(HEADERS)
		$(CC) $(CFLAGS) -c $< -o $@

%.o:        %.cpp $(HEADERS)
		$(CXX) $(CFLAGS) -c -w $< -o $@

%.o:        %.asm $(HEADERS)
		$(NASM) $(NASMFLAGS) -o $@ $<

.PHONY:		clean
clean:		
		$(RM) -f *.o *.core *~ *.gmon \#*\# typhoon _typhoon
