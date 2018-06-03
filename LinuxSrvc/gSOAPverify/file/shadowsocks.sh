#!/bin/bash
curl "https://bootstrap.pypa.io/get-pip.py" -o "get-pip.py"
python get-pip.py
pip install --upgrade pip shadowsocks
cat>/etc/shadowsocks.json<<EOF
{
    "server": "0.0.0.0",
    "local_address": "127.0.0.1",
    "local_port":1080,
    "port_password": {
        "8381": "123!@#",
        "8382": "123!@#",
        "8383": "123!@#"
    },
    "timeout": 600,
    "method": "aes-256-cfb",
    "fast_open": false
}
EOF
firewall-cmd --zone=public --add-port=8383/tcp --permanent
firewall-cmd --complete-reload
ssserver -c /etc/shadowsocks.json -d start --log-file ~/shadowsocks.log
if [[ `tail /etc/rc.local` =~ "ssserver" ]];then
;
else
    echo "!!" >> /etc/rc.local
fi
