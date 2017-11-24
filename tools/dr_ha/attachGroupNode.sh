#bin/bash

SEQPATH="/opt/sequoiadb"
SDB=${SEQPATH}"/bin/sdb"

# run command
$SDB -e "var SEQPATH = \"${SEQPATH}\" ; var CUROPR = 'attachGroupNode'; " -f cluster_opr.js
