#!/bin/bash
##########################################################################
#@decript:     reservation script for upgrade preUninstallationActionList
##########################################################################

# Get sequoiadb install dir
function getInstallDir()
{
    filePath="/etc/default/sequoiadb";
    dirStr=$(grep INSTALL_DIR $filePath)
    local dir=${dirStr#*=}
    echo $dir > test.log
    echo $dir
}

# Stop the stp process
function stopStp()
{
    local dir=$1

    cmdStr="$dir/bin/stpstop"
    eval $cmdStr
    return $?
}

# Stop the stp process before unistalling begins
instalDir=$(getInstallDir)
stopStp "$instalDir" 1>/dev/null
if (($?==0))
then
    echo true;
else
    echo false;
fi
exit 0;
