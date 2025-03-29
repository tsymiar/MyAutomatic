#!/bin/zsh
export SEOXT=1
if [[ `cat /etc/pf.conf | grep rslv` == "" ]]; then
    sudo \cp -rf ./pf.conf.1 /etc/pf.conf; 
fi
sudo pfctl -ef /etc/pf.conf

