#!/bin/bash
if [ "${1}" == "-i" ]; then
	echo "import .sql file"
	mysqldump -u root -p myautomatic < myautomatic.sql
	exit "OK"
else
	echo "kill mysql"
	sudo mysqld stop
	sudo chown -R mysql:mysql /var/lib/mysql
	sudo ln -s /var/lib/mysql/mysql.sock /tmp/mysql.sock
	echo "restart mysql"
	sudo /etc/init.d/mysql start
	echo "run mysqld_safe"
	sudo mysqld_safe --user=mysql --skip-grant-tables --skip-networking
	echo "login mysql with password"
	sudo mysql -u root -p
fi
