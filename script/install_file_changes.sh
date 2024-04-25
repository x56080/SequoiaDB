#!/bin/bash
####################################################################
# Decription:
#   1. Change the owner of sdbomtool to root:root
#   2. Change the permission of sdbomtool to 6755 
####################################################################

function getRootPath()
{
   confFile="/etc/default/sequoiadb"
   if [ -f $confFile ]; then
      source $confFile
   else
      echo "ERROR: File $confFile not exist!"
      exit 1
   fi
}

function changeOMTool()
{
   if [ -f "$INSTALL_DIR/bin/sdbomtool" ]; then
      toolPath=$INSTALL_DIR/bin/sdbomtool
   else
      echo "ERROR: File sdbomtool not exist!"
      exit 1
   fi

   chown root:root $toolPath
   ret=$?
   test $ret -ne 0 && exit $ret
   chmod 6755 $toolPath
   ret=$?
   test $ret -ne 0 && exit $ret
}

function main()
{
   getRootPath
   changeOMTool
   exit 0

}

main
