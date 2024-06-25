#!/bin/bash
echo This may take time - please be patient
echo
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make release
echo
echo The final executable is in the build folder
