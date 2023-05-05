#!/bin/bash



/opt/sequoiadb/bin/sdbimprt --hosts localhost:10000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB1.11820
/opt/sequoiadb/bin/sdbimprt --hosts localhost:11000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB2.11820
/opt/sequoiadb/bin/sdbimprt --hosts localhost:12000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB3.11820

/opt/sequoiadb/bin/sdbimprt --hosts localhost:10000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB1.11830
/opt/sequoiadb/bin/sdbimprt --hosts localhost:11000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB2.11830
/opt/sequoiadb/bin/sdbimprt --hosts localhost:12000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB3.11830

/opt/sequoiadb/bin/sdbimprt --hosts localhost:10000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB7.12840
/opt/sequoiadb/bin/sdbimprt --hosts localhost:11000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB8.12840
/opt/sequoiadb/bin/sdbimprt --hosts localhost:12000 -c IBSFLOW -l FILE_2020 --type json --file /root/packet/ibsflow_meta/SCIMSDB9.12840
