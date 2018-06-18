#!/bin/bash

# dir of this script
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd $DIR

easy_jit_dir=../../
export PYTHONIOENCODING="utf-8"

cat README.md.in | \
  python3 ${easy_jit_dir}/misc/doc/python.py | \
  python3 ${easy_jit_dir}/misc/doc/include.py
