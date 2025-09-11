#!/bin/bash

# Stop at errors.
set -e


compiler=g++
GameLib="game.so"

CC="-O2 -g -ggdb -std=c++17 -fno-exceptions -msse4.2 -maes -Wall -Wno-format -Wno-switch -Wno-write-strings -Wno-multichar -Wno-unused-function -Wno-unused-variable -Wno-missing-braces"
if [ "$compiler" == "g++" ]; then
    CC="$CC -Warray-bounds=2 -Wno-unused-but-set-variable -Wno-strict-aliasing -fpermissive"
else
    CC="$CC -Wno-writable-strings -Wno-braced-scalar-init"
fi

CD="-D__DEVELOPER=1 -D__LINUX=1"
CL="-pthread -lX11 -ldl"

curDir=$(pwd)
buildDir="$curDir/../build"
dataDir="$curDir/../data"

[ -d $buildDir ] || mkdir -p $buildDir
[ -d $dataDir ] || mkdir -p $dataDir

pushd $buildDir > /dev/null

echo Building Game Lib...
$compiler -shared -fPIC $CC $CD "$curDir/game.cpp" -o build_$GameLib
LastError=$?

[ -f $GameLib ] && rm $GameLib
mv build_$GameLib $GameLib

echo Building Linux Platform...
$compiler $CC $CD "$curDir/linux.cpp" -o platform_linux $CL

popd > /dev/null
