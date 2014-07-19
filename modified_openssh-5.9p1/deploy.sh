#!/bin/sh

folder=/home/collector/
origin=/var/tmp/distribute/

cp ${origin}*_ips.txt ${folder}
cp ${origin}Makefile ${folder}
cp ${origin}capfi*.cpp ${folder}
cp ${origin}runfi*.rb ${folder}

mkdir ${folder}.ssh/
cp ${origin}local_rsa ${folder}.ssh/

cp -r ${origin}Onload* $folder
cp -r ${origin}firefox $folder
cp -r ${origin}mysql-ruby-2.8.2 $folder

cp -r ${origin}modified* $folder
cp ${origin}clusters.txt /var/tmp/
cp ${origin}st.txt /var/tmp/

mkdir /var/tmp/sshlogs
chmod 0777 /var/tmp/sshlogs
mkdir ${folder}noscript_st_1000_100_local
chmod 0777 ${folder}noscript_st_1000_100_local

sudo apt-get install gedit libpcap-dev tshark libssl-dev ruby ruby1.8-dev ruby1.9.1-dev rubygems1.8  mysql-client libmysqlclient-dev libopenssl-ruby xvfb dbus-x11
