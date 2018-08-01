#!/bin/sh
touch main.c
opts='GENETIC=1 PERF_COUNTERS=1 MP=1 '
if expr $OSTYPE = "darwin"; then
  opts=${opts}' OSX=1 CC=gcc' 
  make -j5 $opts $*
else
  opts=${opts}' CC=gcc'
  gmake -j5 $opts $*
fi
strip ./typhoon
