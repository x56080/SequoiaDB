#!/bin/bash
BashPath=$(dirname $(readlink -f $0))

pwdpath=$(pwd)
SYS_CONF_FILE=/etc/default/sequoiadb
USER=sdbadmin
cur_user=`whoami`
confrootpath="$BashPath/../conf/local"
logrootpath="$BashPath/../log"
  
function check_user()
{
  . $SYS_CONF_FILE


  if [ -n "$SDBADMIN_USER" ];then
    USER=$SDBADMIN_USER
    if [ "$cur_user" != "$SDBADMIN_USER" -a "$cur_user" != "root" ]; then
      echo "ERROR: fsstart requires USER [$USER] permission"
      exit 129
    fi
  else
    local author=`ls -l $BashPath/fsstart.sh | awk '{print $3}'`
    USER=$author
    if [ "$cur_user" != "$author" -a "$cur_user" != "$root" ]; then
      echo "ERROR: fsstart requires USER [$USER] permission"
      exit 129
    fi
  fi
}


check_user

if [ "$cur_user" == "root" ]; then
  if [ -d "$confrootpath" ]; then
    chown $USER -R "$confrootpath"
  fi
  if [ -d "$logrootpath" ]; then
    chown $USER -R "$logrootpath"
  fi 
  su $USER -c "./start_i.sh $*"
else
  ./start_i.sh $*
fi  
