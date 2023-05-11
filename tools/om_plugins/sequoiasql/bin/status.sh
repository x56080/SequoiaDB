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
   echo "$binFileName is not started";
   exit 1;
else
   echo "$binFileName is already started ($pid)";
   exit 0;
fi
