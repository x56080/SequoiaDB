#!/bin/bash
function getStpNode()
{
   local STPHOSTNAME=$1
   local STPSVCNAME=$2
   local FUNCTION=$3
   local SCRIPTDIR=$4
   cd $SCRIPTDIR
   local rc=`/opt/sequoiadb/bin/sdb -f StpUtil.js  -e "var STPHOSTNAME='$STPHOSTNAME';var STPSVCNAME='$STPSVCNAME'; var FUNCTION='$FUNCTION'";`
   echo $rc
}
getStpNode $1 $2 $3 $4