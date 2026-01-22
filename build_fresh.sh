#!/bin/bash
cd /home/dj/PurpleTeam/3snake/3snake
make clean > /dev/null 2>&1
make
echo "Build complete"
ls -lh ./3snake
