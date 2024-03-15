#bin/bash

SCRIPT_DIR=$(dirname "$0")
SEQPATH="`cd $SCRIPT_DIR && pwd`/../../"
SDB=${SEQPATH}"bin/sdb"
CLUSTER_OPR_PATH=${SEQPATH}"/tools/dr_ha/cluster_opr.js"

# run command
$SDB -e "var SEQPATH = \"${SEQPATH}\" ; var CUROPR = 'attachGroupNode'; " -f ${CLUSTER_OPR_PATH}
