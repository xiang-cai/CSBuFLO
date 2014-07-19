#!/bin/sh

kill -9 $(ps ax | grep "ruby" | sed -r 's/^[ \t]+//g'| cut -f1 -d" ")
sleep 1

kill -9 $(ps ax | grep "local_rsa" | sed -r 's/^[ \t]+//g'| cut -f1 -d" ")
sleep 1

kill -9 $(ps ax | grep "modssh" | sed -r 's/^[ \t]+//g'| cut -f1 -d" ")
sleep 1

kill -9 $(ps ax | grep "tshark" | sed -r 's/^[ \t]+//g'| cut -f1 -d" ")
sleep 1

kill -9 $(ps ax | grep "vfb" | sed -r 's/^[ \t]+//g'| cut -f1 -d" ")

#rm -rf /tmp/*

