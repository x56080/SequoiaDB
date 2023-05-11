#!/bin/bash

#init variable
currentPath=$(cd "$(dirname "$0")"; pwd)

#config parameter
binName="sequoiasql-plugin"
extName="jar"


#include common.sh
source "$currentPath/common.sh"

binFileName=`getExecFileName $currentPath $binName $extName`
if [ "$binFileName"x == ""x ]; then
   echo "Error: executable program not found.";
   exit 1;
fi

#echo $binFileName

omhttpname=`getConfig $currentPath "omhttpname"`
if [ "$omhttpname"x == ""x ]; then
   omhttpname="8000"
fi

#echo $omhttpname

pid=`getProcId $binFileName "--__omhttpname=$omhttpname"`
if [ "$pid"x == ""x ]; then
   echo "$binFileName is not started" ;
   exit 0;
else
   kill $pid

   for i in $(seq 1 10)
   do
      sleep 1s

      tmp=`getProcId $binFileName "--__omhttpname=$omhttpname"`
      if [ "$tmp"x == ""x ]; then
         echo "Success: $binFileName is successfully stoped ($pid)" ;
         exit 0
      fi
   done

   echo "Failed to stop $binFileName ($pid)" ;
   exit 1
fi

