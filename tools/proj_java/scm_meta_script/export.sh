#!/bin/bash

/opt/sequoiadb/bin/sdbexprt --hosts localhost:10000 -c IBSFLOW -l FILE_2020 --withid false --type json --file /root/packet/ibsflow_meta/node1.json
/opt/sequoiadb/bin/sdbexprt --hosts localhost:11000 -c IBSFLOW -l FILE_2020 --withid false --type json --file /root/packet/ibsflow_meta/node2.json
/opt/sequoiadb/bin/sdbexprt --hosts localhost:12000 -c IBSFLOW -l FILE_2020 --withid false --type json --file /root/packet/ibsflow_meta/node3.json
