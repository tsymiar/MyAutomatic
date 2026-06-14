#!/bin/bash
git config --global pack.threads "4"
git config --global pack.packSizeLimit 2g
git config --global fetch.parallel 8
git config --global core.preloadIndex true
git config --global core.fscache true
git config --global gc.auto 256
git config --global http.lowSpeedLimit 0
git config --global http.lowSpeedTime 999999
git config --global http.postBuffer 524288000
git config --global core.compression 9
