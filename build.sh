#!/bin/bash
cd ./node_modules/Platinum/Platinum
scons
cd ../../..
cp ./node_modules/Platinum/Platinum/Build/Targets/universal-apple-macosx/Debug/*.a ./
cp ./node_modules/Platinum/Platinum/Build/Targets/x86-unknown-linux/*.a ./
make
node-gyp configure
node-gyp build
cp build/Release/media_watcher.node ./
