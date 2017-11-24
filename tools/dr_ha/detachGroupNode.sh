#bin/bash

SEQPATH="/opt/sequoiadb"
SDB=${SEQPATH}"/bin/sdb"

# run command
$SDB -e "var SEQPATH = \"${SEQPATH}\" ; var CUROPR = 'detachGroupNode'; " -f cluster_opr.js
