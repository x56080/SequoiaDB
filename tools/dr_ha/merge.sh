#bin/bash

SEQPATH="/opt/sequoiadb"
SDB=${SEQPATH}"/bin/sdb"

# run command
$SDB -e "var SEQPATH = \"${SEQPATH}\" ; var CUROPR = 'merge'; " -f cluster_opr.js
