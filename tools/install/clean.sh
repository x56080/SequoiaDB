#!/bin/bash

TYPE="unknown"
CLEAN_PG=0
CLEAN_DB=0
CLEAN_MYSQL=0
FORCE=false
LOCAL=false

function clean_by_dbtype()
{
   local name=$1
   local installInfos=""
   
   if [ $LOCAL == true ];
   then
      installInfos=`find /etc/default -regex ".*$name[1-4][0-9]" -o -regex ".*$name[1-9]" -o -name "$name"`
   else
      installInfos=`cat /etc/default/sequoiadb-setup.list | grep "$name"`
   fi
   #local=true: installInfo record the installed config file in the /etc/default, eg: "/etc/default/sequoiadb"
   #local=false: installInfo record the installed config file and md5 in the /etc/default/sequoiadb-setup.list, eg: "/etc/default/sequoiadb,md5"
   for installInfo in $installInfos
   do
      local file=`echo $installInfo |awk -F, '{print $1}'`
      local md5=`echo $installInfo |awk -F, '{print $2}'`
      [ -z $md5 ] && md5="xx"
      
      . $file
      if [ $LOCAL == true -o $MD5 == $md5 ]; 
      then
         case $name in
            "sequoiadb")
                          clean_sdb $installInfo
                          shift
                          ;;
            "sequoiasql-mysql" | "sequoiasql-postgresql")
                          clean_sql $installInfo
                          shift
                          ;;
            *)            echo "Internal error!"
                          exit 64
                          ;;
         esac
      fi
   done
}

function clean_sdb()
{
   local installInfo=$1
   local file=`echo $installInfo |awk -F, '{print $1}'`
   
   . $file
   #local=true: filter record the config file under /etc/default, eg: "sequoiadb"
   #local=false: filter record the installed config file and md5 in the /etc/default/sequoiadb-setup.list, eg: "sequoiadb,md5"
   local filter=`echo $installInfo |awk -F / '{print $4}'`
   if [ $FORCE == false ];then
      read -p "clean $INSTALL_DIR $name Y/n: " choice
   fi
   
   [ -z $choice ] && choice="Y"
   if [[ "$choice" == "Y" || "$choice" == "y" ]];then
      
      local datadir_list=`$INSTALL_DIR/bin/sdblist -l | grep -v "Total"|awk 'NR>1{print $NF}'`
      echo "begin to uninstall $name"
      echo "$INSTALL_DIR/uninstall --mode unattended"
      `$INSTALL_DIR/uninstall --mode unattended`
      test $? -ne 0 && { echo "ERROR: Fail to $INSTALL_DIR/uninstall --mode unattended" >&2 && exit 1; }
      
      echo "ok"
      for datadir in $datadir_list
      do
         echo "rm -rf $datadir"
         rm -rf $datadir
      done
      echo "begin to clean install dir"
      echo "rm -rf $INSTALL_DIR"
      rm -rf $INSTALL_DIR
      
      `sed -i '/'$filter'/d' /etc/default/sequoiadb-setup.list`
      echo "ok"
   fi
   return 0
}

function clean_sql()
{
   local installInfo=$1
   local file=`echo $installInfo |awk -F, '{print $1}'`
   
   . $file
   #local=true: filter record the config file under /etc/default, eg: "sequoiasql-mysql"
   #local=false: filter record the installed config file and md5 in the /etc/default/sequoiadb-setup.list, eg: "sequoiasql-mysql,md5"
   local filter=`echo $installInfo |awk -F / '{print $4}'`
   if [ $FORCE == false ];then
      read -p "clean $INSTALL_DIR $name Y/n: " choice
   fi 
   
   [ -z $choice ] && choice="Y"
   if [[ "$choice" == "Y" || "$choice" == "y" ]];then

      local datadir_list=`$INSTALL_DIR/bin/sdb_sql_ctl listinst | grep -v "Total"|awk 'NR>1{print $2 " " $3}'`
      echo "begin to uninstall $name"
      echo "$INSTALL_DIR/uninstall --mode unattended"
      `$INSTALL_DIR/uninstall --mode unattended`
      test $? -ne 0 && { echo "ERROR: Fail to $INSTALL_DIR/uninstall --mode unattended" >&2 && exit 1; }
      
      echo "ok"
      for datadir in $datadir_list
      do
         echo "rm -rf $datadir"
         rm -rf $datadir
      done
      echo "begin to clean install dir"
      echo "rm -rf $INSTALL_DIR"
      rm -rf $INSTALL_DIR
      
      `sed -i '/'$filter'/d' /etc/default/sequoiadb-setup.list`
      echo "ok"
   fi
      
   return 0
}

function clean_all()
{
   clean_by_dbtype "sequoiadb"
   clean_by_dbtype "sequoiasql-mysql"
   clean_by_dbtype "sequoiasql-postgresql"
}

function build_help()
{
   echo ""
   echo "Usage:"
   echo "  --sdb        clean sequoiadb"
   echo "  --pg         clean sequoiasql-postgresql"
   echo "  --mysql      clean sequoiasql-mysql"
   echo "  --force      don't ask user when clean sdb, mysql or pg"
   echo "  --local      clean up all local install and data"
}

#Parse command line parameters
#test $# -eq 0 && { build_help && exit 64; }

ARGS=`getopt -o h --long help,sdb,pg,mysql,force,local -n 'test' -- "$@"`
ret=$?
test $ret -ne 0 && exit $ret

eval set -- "${ARGS}"

while true
do
   case "$1" in
      --sdb )          TYPE="sequoiadb"
                       CLEAN_DB=1
                       shift
                       ;;
      --mysql )        TYPE="sequoiasql-mysql"
                       CLEAN_MYSQL=1
                       shift
                       ;;
      --pg )           TYPE="sequoiasql-postgresql"
                       CLEAN_PG=1
                       shift
                       ;;
      --force )        FORCE=true
                       shift
                       ;;
      --local )        LOCAL=true
                       shift
                       ;;
      -h | --help )    build_help
                       exit 0
                       ;;
      --)              shift
                       break
                       ;;
      *)               echo "Internal error!"
                       exit 64
                       ;;
   esac
done

if [ $CLEAN_DB -ne 0 ];then
   clean_by_dbtype "sequoiadb"
fi

if [ $CLEAN_MYSQL -ne 0 ];then
   clean_by_dbtype "sequoiasql-mysql"
fi

if [ $CLEAN_PG -ne 0 ];then
   clean_by_dbtype "sequoiasql-postgresql"
fi

case "$TYPE" in
   unknown)             clean_all; shift 1;;
         *)             shift 1;;
esac