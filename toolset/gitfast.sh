#!/bin/bash
GIT_TRACE_CURL=1 git fetch 2>&1 | head -40
git config --global pack.threads "4"
git config --global pack.packSizeLimit 2g
git config --global fetch.parallel 8
git config --global core.preloadIndex true
git config --global core.fscache true
git config --global gc.auto 256
git config --global http.lowSpeedLimit 0
git config --global http.lowSpeedTime 999999
git config --global http.postBuffer 524288000
git config --global protocol.version 2
git config --global http.maxRequests 16
git config --global submodule.fetchJobs 8
git config --global fetch.prune true
git config --global fetch.writeCommitGraph true
git config --global core.commitGraph true
git config --global core.compression 0
git config --global http.sslVerify true
# fatch little tag / https to SSH
# git config --global remote.origin.tagOpt --no-tags
# git config --global url."git@github.com:".insteadOf "https://github.com/"
