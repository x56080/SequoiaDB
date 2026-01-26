#!/bin/bash
res=$(find ./jars/ -maxdepth 1 -name 'sdb-schedule-tools-*.jar')
res_count=$(echo ${res} | wc -w)
echo $res
if [ $(echo $res_count) = '1' ] ;then
  exit 0
else
  exit 1
fi