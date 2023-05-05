#!/bin/bash

addr=$1
csName=$2

#addr="192.168.30.35:11810"
#csName="IBSFLOW2"


if [ $# -ne 2 ]; then
   echo "ERROR: need 2 arguments"
   exit -1
fi

/opt/sequoiadb/bin/sdbinspect --coord ${addr} -a inspect -c ${csName} -l FILE_2020 -o ${csName}.FILE_2020.insp
