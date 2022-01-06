#!/bin/bash
BashPath=$(dirname $(readlink -f $0))

pwdpath=$(pwd)
SYS_CONF_FILE=/etc/default/sequoiadb
USER=sdbadmin
cur_user=`whoami`
confrootpath="$BashPath/../conf/local"
logrootpath="$BashPath/../log"
startfs="$BashPath/start_i.sh"

function Usage()
{
    $startfs "--help"
}
  
function check_user()
{
  if [ -f "$SYS_CONF_FILE" ]; then
    . $SYS_CONF_FILE
  else
    echo "ERROR: $SYS_CONF_FILE does not exist, find default user failed. Use the --currentuser parameter to ignore default user"
    exit 1
  fi

  if [ -n "$SDBADMIN_USER" ];then
    USER=$SDBADMIN_USER
    if [ "$cur_user" != "$SDBADMIN_USER" -a "$cur_user" != "root" ]; then
      echo "ERROR: fsstart requires user [$USER] default, switch to default user or use the --currentuser parameter to ignore default user"
      exit 126
    fi
  else
    echo "ERROR: SDBADMIN_USER is not in $SYS_CONF_FILE, check $SYS_CONF_FILE or use the --currentuser parameter to ignore default user"
    exit 125
  fi
}

useCurUser=""
for arg in "$@"
do
   case $arg in
   --currentuser)
      useCurUser="true"
      ;;
   -h|--help)
      Usage
      exit 0
      ;;
   *)	
      ;;   
   esac
done 

if [ -z "$useCurUser" ]; then
   check_user

   if [ "$cur_user" == "root" ]; then
     su - $USER -c "$startfs $*"
   else
     $startfs $*
   fi
else
   $startfs $*
fi 
