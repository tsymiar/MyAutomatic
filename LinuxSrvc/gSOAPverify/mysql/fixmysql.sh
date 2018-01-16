#!/bin/sh
if [ "${1}" == "-i" ]; then
	echo "导入数据库"
	mysqldump -u root -p custominfo > custominfo.sql
	echo "OK"
	exit 0
else
	echo 停止mysql
	sudo mysqld stop
	sudo chown -R mysql:mysql /var/lib/mysql
	sudo ln -s /var/lib/mysql/mysql.sock /tmp/mysql.sock
	echo 重启mysql
	sudo /etc/init.d/mysql start
	echo 启动mysqld_safe
	sudo mysqld_safe --user=mysql --skip-grant-tables --skip-networking
	echo 输入mysql密码
	sudo mysql -u root -p
fi
