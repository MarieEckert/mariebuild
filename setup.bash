#!/bin/bash

MCFG_2_TAG="0.4.3"

function directory_setup() {
  if [ -d lib ]; then
    rm -rf lib
  fi

  if [ -d obj ]; then
    rm -rf obj
  fi

  mkdir lib
  mkdir obj
  mkdir include
}

function libmcfg_2_setup() {
  BUILD_DIR=".libmcfg_2.build"
  if [ -d $BUILD_DIR ]; then
    rm -rf $BUILD_DIR
  fi

  mkdir $BUILD_DIR
  cd $BUILD_DIR

  git clone https://github.com/FelixEcker/mcfg_2.git

  cd mcfg_2
  
  echo "==> working on tag $MCFG_2_TAG"
  git checkout $MCFG_2_TAG 2> /dev/null
  
  bash scripts/setup.bash || exit
  bash scripts/build.bash --lib-only || return

  mv include/* ../../include/
  mv libmcfg_2.a ../../lib/
  cd ../..

  rm -rf $BUILD_DIR
}

echo "MB setup script. "
echo "==> setting up directories"
directory_setup
echo "==> building libraries"
libmcfg_2_setup
