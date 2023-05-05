#!/bin/bash

loop=$1

if [ $# -ne 1 ]; then
   echo "ERROR: need 1 argument"
   exit -1
fi

# import by data node
/opt/sequoiadb/bin/sdbimprt --hosts localhost:10000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB1.11820 > out.log
/opt/sequoiadb/bin/sdbimprt --hosts localhost:11000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB2.11820 >> out.log
/opt/sequoiadb/bin/sdbimprt --hosts localhost:12000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB3.11820 >> out.log

/opt/sequoiadb/bin/sdbimprt --hosts localhost:10000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB1.11830 >> out.log
/opt/sequoiadb/bin/sdbimprt --hosts localhost:11000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB2.11830 >> out.log
/opt/sequoiadb/bin/sdbimprt --hosts localhost:12000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB3.11830 >> out.log

/opt/sequoiadb/bin/sdbimprt --hosts localhost:10000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB7.12840 >> out.log
/opt/sequoiadb/bin/sdbimprt --hosts localhost:11000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB8.12840 >> out.log
/opt/sequoiadb/bin/sdbimprt --hosts localhost:12000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB9.12840 >> out.log

# import by coord
echo "begin: "`date`
for ((i=1; i<=$loop; i++))
do
    echo "round ${i}..."
    /opt/sequoiadb/bin/sdbimprt --hosts localhost:11810 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/node1.json >> out.log
    /opt/sequoiadb/bin/sdbimprt --hosts localhost:11810 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/node2.json >> out.log
    /opt/sequoiadb/bin/sdbimprt --hosts localhost:11810 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/node3.json >> out.log
done
echo "end: "`date`
