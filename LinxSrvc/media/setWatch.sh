#!/bin/bash
# Desc: Set the environment for the video decoder
export DISPLAY=:0
xhost +local:
# Set the permission for the video decoder
sudo chmod 666 /dev/video0
sudo chmod 666 /dev/nv*
echo 1024 | sudo tee /proc/sys/vm/nr_hugepages
# Check the video decoder status
v4l2-ctl -d /dev/video0 --list-formats-ext
watch -n 1 "cat /proc/driver/nvidia/decoder/status"
