expect<<EOF
set timeout 1
spawn ./8927.sh
expect "*assword:"
send "xxxxx\n"
interact
exit
EOF
