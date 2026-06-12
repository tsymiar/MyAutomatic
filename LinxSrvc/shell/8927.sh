o#!/bin/zsh
export SEOXT=1
if ! grep -q rslv /etc/pf.conf 2>/dev/null; then
    sudo cp -f ./pf.conf.1 /etc/pf.conf
fi
sudo pfctl -ef /etc/pf.conf

