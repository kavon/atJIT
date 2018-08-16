#!/bin/bash

# expected input: POSIX standard time output (-p option)
# crashes if $1 is not less than $2

PAT='s/real //'

A=`head -n1 $1 | sed "$PAT"`
B=`head -n1 $2 | sed "$PAT"`

if [[ `echo "$A >= $B" | bc` -eq 1 ]]; then
  echo "$A >= $B"
  exit 1
fi

echo "$A < $B"
