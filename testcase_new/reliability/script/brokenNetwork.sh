#!/bin/bash
trap "" 1 2 3 24
eth=`ifconfig | grep 'eth' | awk '{print $1}'`
ifconfig ${eth} down
sleep $1 &
wait
ifconfig ${eth} up
service network restart
/etc/init.d/networking restart