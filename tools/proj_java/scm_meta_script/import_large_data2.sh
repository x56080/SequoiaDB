#!/bin/bash

addr=$1
csName=$2
loop=$3

#addr="192.168.30.35:11810"
#csName="IBSFLOW2"
#loop=3


if [ $# -ne 3 ]; then
   echo "ERROR: need 3 arguments"
   exit -1
fi

# import by coord
echo "begin: "`date`
for ((i=1; i<=$loop; i++))
do
    echo "round ${i}..."
    /opt/sequoiadb/bin/sdbimprt --hosts ${addr} -c ${csName} -l FILE_2020 --type json --file /root/packet/ibsflow_meta/node1.json >> out.log
    /opt/sequoiadb/bin/sdbimprt --hosts ${addr} -c ${csName} -l FILE_2020 --type json --file /root/packet/ibsflow_meta/node2.json >> out.log
    /opt/sequoiadb/bin/sdbimprt --hosts ${addr} -c ${csName} -l FILE_2020 --type json --file /root/packet/ibsflow_meta/node3.json >> out.log
done
echo "end: "`date`
