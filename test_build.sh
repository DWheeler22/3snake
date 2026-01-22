#!/bin/bash
cd /home/dj/PurpleTeam/3snake/3snake
echo "=== Cleaning ==="
make clean > /dev/null 2>&1

echo "=== Building ==="
make

echo "=== Build status ==="
if [ -f ./3snake ]; then
    echo "Binary successfully created!"
    file ./3snake
else
    echo "ERROR: Binary not created!"
fi
