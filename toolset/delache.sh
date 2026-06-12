#!/bin/bash
sudo apt-get clean
sudo apt-get autoclean
sudo apt-get autoremove -y
pip cache purge
