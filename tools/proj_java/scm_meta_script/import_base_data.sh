#!/bin/bash

# 40000/41000/42000 分别为同一个group的三个节点的端口

/opt/sequoiadb/bin/sdbimprt --hosts localhost:40000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB1.11820
/opt/sequoiadb/bin/sdbimprt --hosts localhost:41000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB2.11820
/opt/sequoiadb/bin/sdbimprt --hosts localhost:42000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB3.11820

/opt/sequoiadb/bin/sdbimprt --hosts localhost:40000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB1.11830
/opt/sequoiadb/bin/sdbimprt --hosts localhost:41000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB2.11830
/opt/sequoiadb/bin/sdbimprt --hosts localhost:42000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB3.11830

/opt/sequoiadb/bin/sdbimprt --hosts localhost:40000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB7.12840
/opt/sequoiadb/bin/sdbimprt --hosts localhost:41000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB8.12840
/opt/sequoiadb/bin/sdbimprt --hosts localhost:42000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB9.12840
