#!/bin/bash
cd /home/dj/PurpleTeam/3snake/3snake
make clean > /tmp/build.log 2>&1
make >> /tmp/build.log 2>&1
echo "Build completed. Exit code: $?" >> /tmp/build.log
ls -la 3snake >> /tmp/build.log 2>&1
