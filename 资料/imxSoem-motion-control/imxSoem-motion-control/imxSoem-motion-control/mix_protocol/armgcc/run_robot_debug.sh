#!/bin/sh
set -eu
cd /home
rm -f /tmp/robot_main.out /tmp/robot_main.rc
if timeout 20s python3 -u /home/main.py > /tmp/robot_main.out 2>&1; then
  echo 0 > /tmp/robot_main.rc
else
  echo $? > /tmp/robot_main.rc
fi