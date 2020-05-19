#!/bin/bash
BashPath=$(dirname $(readlink -f $0))

pwdpath=$(pwd)
SYS_CONF_FILE=/etc/default/sequoiadb
USER=sdbadmin
function check_user()
{
  . $SYS_CONF_FILE

  local cur_user=`whoami`
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
  echo $USER
}


check_user

su $USER -c "cd $BashPath; pwd; ./start_i.sh $*"
