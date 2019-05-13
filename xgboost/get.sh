#!/usr/bin/env bash

set -ex

# from: https://stackoverflow.com/questions/59895/getting-the-source-directory-of-a-bash-script-from-within
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

ROOT="root"

pushd $DIR

rm -rf $ROOT
mkdir $ROOT

git clone -b 'v0.72' --single-branch --depth 1 --recursive https://github.com/dmlc/xgboost.git $ROOT

pushd $ROOT

# ./build.sh
make -j 4
