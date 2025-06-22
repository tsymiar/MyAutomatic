sudo codesign -fs "gdb-cert-self-signed" /usr/local/bin/gdb
sudo killall taskgated
