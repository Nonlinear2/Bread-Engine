#!/bin/bash
echo Remember that you need Cmake and a C compiler installed
echo For Debian based distros you can do that with the following commands
echo "sudo apt install cmake"
echo and
echo "sudo apt install build-essentials"
echo Run this script again when you are done
echo
echo This may take time please be patient
echo Report errors over github
echo
mkdir build
cd build
cmake ..
make release
echo
echo The final executable is in build called bread_engine_version
ls | grep bread_engine_
echo To run just type ./bread_engine_version
echo Then type in one by one the commands
echo
echo uci
echo position startpos
echo go movetime 5000
echo
echo For the gui head over to cute chess, link is readme.md
echo Install the application
echo To add the engine select tools on the top bar
echo Then click on settings then on engines
echo Then click on + at the bottom the add the engine
echo Click on browse next where it says Command
echo Choose the executable bread_engine_version
echo
echo To play a game press game then new, select Human vs CPU.


