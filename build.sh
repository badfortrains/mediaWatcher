#!/bin/bash
cd ./Platinum/Platinum
scons
cd ../..
cp Platinum/Platinum/Build/Targets/universal-apple-macosx/Debug/*.a ./
cp Platinum/Platinum/Build/Targets/x86-unknown-linux/*.a ./
make
node-gyp configure
node-gyp build
cp build/Release/media_watcher.node ./
