#!/bin/bash
export HOST=x86_64-w64-mingw32
export CXX=x86_64-w64-mingw32-g++-posix
export CC=x86_64-w64-mingw32-gcc-posix
export PREFIX="$(pwd)/depends/$HOST"

set -eu -o pipefail

set -x
cd "$(dirname "$(readlink -f "$0")")/.."

cd depends/ && make HOST=$HOST V=1 NO_QT=1
cd ../
WD=$PWD

cd src/cc
echo $PWD
echo Making cclib...
./makecustom

cd ./priceslibs
echo Making prices feeds custom libs...
CC="${CC} -g " CXX="${CXX} -g " make dll

cd $WD

./autogen.sh
CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site CXXFLAGS="-DPTW32_STATIC_LIB -DCURL_STATICLIB -DCURVE_ALT_BN128 -fopenmp -pthread" ./configure --prefix="${PREFIX}" --host=x86_64-w64-mingw32 --enable-static --disable-shared
sed -i 's/-lboost_system-mt /-lboost_system-mt-s /' configure
cd src/
CC="${CC} -g " CXX="${CXX} -g " make V=1  komodod.exe komodo-cli.exe komodo-tx.exe

cd $WD/src/cc/dapps/
make subatomic_win