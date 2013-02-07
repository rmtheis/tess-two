#!/bin/bash

#########################################
# build type (arm/x86)
#########################################

CPU="$1$2"
if [ "$2" == "v7le" ]; then
  CPUDIR="$1le-v7"
  BUSUFFIX="$1v7"
elif [ "$2" == "a9" ]; then
  CPUDIR="$1le-v7"
  CPU="$1v7le"
  BUSUFFIX="$1v7"
  CPUGEN="a9"
else
  CPUDIR="$CPU"
  BUSUFFIX="$1"
fi

#########################################
# variables
#########################################

TARGET_LIB="$QNX_TARGET/$CPUDIR/lib"
TARGET_USR_LIB="$QNX_TARGET/$CPUDIR/usr/lib"
TARGET_INC="$QNX_TARGET/usr/include"

COMP_PATHS=" \
    -L$TARGET_LIB \
    -L$TARGET_USR_LIB \
    -I$TARGET_INC"

export CFLAGS="-fpic $COMP_PATHS $MAKEFLAGS"
export CXXFLAGS="-fpic $COMP_PATHS $MAKEFLAGS"
export CPPFLAGS="-fpic $COMP_PATHS $MAKEFLAGS"

export CPP="$QNX_HOST/usr/bin/nto$BUSUFFIX-cpp-4.6.3"
export CXX="$QNX_HOST/usr/bin/nto$BUSUFFIX-g++-4.6.3"
export CXXCPP="$QNX_HOST/usr/bin/nto$BUSUFFIX-cpp-4.6.3"
export CC="$QNX_HOST/usr/bin/nto$BUSUFFIX-gcc-4.6.3"
export LD="$QNX_HOST/usr/bin/nto$BUSUFFIX-ld"
export AR="$QNX_HOST/usr/bin/nto$BUSUFFIX-ar"
export AS="$QNX_HOST/usr/bin/nto$BUSUFFIX-as"
export NM="$QNX_HOST/usr/bin/nto$BUSUFFIX-nm"
export OBJDUMP="$QNX_HOST/usr/bin/nto$BUSUFFIX-objdump"
export STRIP="$QNX_HOST/usr/bin/nto$BUSUFFIX-strip"
export RANLIB="$QNX_HOST/usr/bin/nto$BUSUFFIX-ranlib"
export LDFLAGS="-L$TARGET_LIB -L$TARGET_USR_LIB"

#########################################
# auto* setup & configure
#########################################

autoreconf -fiv

./configure --prefix=`pwd`/../../leptonica-$1-stage --build=x86_64 --host=arm-qnx --enable-shared=yes --disable-programs --without-zlib --without-libpng --without-jpeg --without-giflib --without-libtiff

#########################################
# build
#########################################

make -j8 && make install
