#!/bin/bash
####################################################################
# Description:
#   Preserve sdbcm.conf during upgrade/cover installation.
#
#   --backup  : save existing sdbcm.conf before package deployment
#   --restore : replace installed file with backup (full restore)
#   --merge   : merge backup into installed file (recommended for upgrade)
#
# Merge strategy:
#   1. Use the new package sdbcm.conf as base (keep new comments/params)
#   2. For each active key=value in backup, override the same key in base
#   3. If backup key only exists as a commented line in base, replace that line
#   4. Append backup-only keys (e.g. OMAddress, <hostname>_Port) at the end
####################################################################

BACKUP_FILE=""
CONF_FILE=""
MERGE_FILE=""
DO_BACKUP=0
DO_RESTORE=0
DO_MERGE=0

function getInstallDir()
{
   if [ -n "$SYS_INSTALL_DIR" ]; then
      INSTALL_DIR="$SYS_INSTALL_DIR"
   elif [ -f /etc/default/sequoiadb ]; then
      source /etc/default/sequoiadb
   else
      echo "ERROR: Cannot determine install directory!"
      exit 1
   fi
}

function initPaths()
{
   getInstallDir
   if [ -n "$SYS_TMP_DIR" ]; then
      BACKUP_FILE="$SYS_TMP_DIR/sdbcm.conf.bak"
      MERGE_FILE="$SYS_TMP_DIR/sdbcm.conf.merge"
   else
      BACKUP_FILE="/tmp/sdbcm.conf.bak"
      MERGE_FILE="/tmp/sdbcm.conf.merge"
   fi
   CONF_FILE="$INSTALL_DIR/conf/sdbcm.conf"
}

function backupConf()
{
   if [ -f "$CONF_FILE" ]; then
      cp -fp "$CONF_FILE" "$BACKUP_FILE"
      ret=$?
      test $ret -ne 0 && exit $ret
   fi
}

function restoreConf()
{
   if [ -f "$BACKUP_FILE" ]; then
      cp -fp "$BACKUP_FILE" "$CONF_FILE"
      ret=$?
      test $ret -ne 0 && exit $ret
   fi
}

function restoreFileMeta()
{
   local owner="$1"
   local mode="$2"

   if [ -n "$owner" ] && [ -f "$CONF_FILE" ]; then
      chown "$owner" "$CONF_FILE"
      ret=$?
      test $ret -ne 0 && exit $ret
   fi
   if [ -n "$mode" ] && [ -f "$CONF_FILE" ]; then
      chmod "$mode" "$CONF_FILE"
      ret=$?
      test $ret -ne 0 && exit $ret
   fi
}

function mergeConf()
{
   local confOwner=""
   local confMode=""

   if [ ! -f "$BACKUP_FILE" ]; then
      return 0
   fi

   if [ ! -f "$CONF_FILE" ]; then
      cp -fp "$BACKUP_FILE" "$CONF_FILE"
      ret=$?
      test $ret -ne 0 && exit $ret
      return 0
   fi

   confOwner=`stat -c '%u:%g' "$CONF_FILE" 2>/dev/null`
   confMode=`stat -c '%a' "$CONF_FILE" 2>/dev/null`

   awk '
   function trim(s)
   {
      sub(/^[ \t\r\n]+/, "", s)
      sub(/[ \t\r\n]+$/, "", s)
      return s
   }

   function getActiveKey(line,    t, pos, key)
   {
      t = trim(line)
      if ( t == "" || substr(t, 1, 1) == "#" )
      {
         return ""
      }
      pos = index(t, "=")
      if ( pos == 0 )
      {
         return ""
      }
      key = substr(t, 1, pos - 1)
      return trim(key)
   }

   function getCommentKey(line,    t, pos, key)
   {
      t = trim(line)
      if ( substr(t, 1, 1) != "#" )
      {
         return ""
      }
      t = trim(substr(t, 2))
      pos = index(t, "=")
      if ( pos == 0 )
      {
         return ""
      }
      key = substr(t, 1, pos - 1)
      return trim(key)
   }

   FNR == NR {
      key = getActiveKey($0)
      if ( key != "" )
      {
         old[key] = trim($0)
      }
      next
   }

   {
      key = getActiveKey($0)
      if ( key != "" && ( key in old ) )
      {
         print old[key]
         delete old[key]
         next
      }

      key = getCommentKey($0)
      if ( key != "" && ( key in old ) )
      {
         print old[key]
         delete old[key]
         next
      }

      print $0
   }

   END {
      for ( key in old )
      {
         print old[key]
      }
   }
   ' "$BACKUP_FILE" "$CONF_FILE" > "$MERGE_FILE"
   ret=$?
   test $ret -ne 0 && exit $ret

   cp -fp "$MERGE_FILE" "$CONF_FILE"
   ret=$?
   if [ $ret -eq 0 ]; then
      rm -f "$MERGE_FILE"
      restoreFileMeta "$confOwner" "$confMode"
   else
      exit $ret
   fi
}

function main()
{
   initPaths

   if [ "$DO_BACKUP" == "1" ]; then
      backupConf
      exit 0
   fi

   if [ "$DO_RESTORE" == "1" ]; then
      restoreConf
      exit 0
   fi

   if [ "$DO_MERGE" == "1" ]; then
      mergeConf
      exit 0
   fi

   echo "Invalid parameters"
   exit 1
}

ARGS=`getopt -o h --long backup,restore,merge -n 'preserve_sdbcm_conf.sh' -- "$@"`
ret=$?
test $ret -ne 0 && exit $ret
eval set -- "${ARGS}"

while true
do
   case "$1" in
      --backup )   DO_BACKUP=1
                   shift 1
                   ;;
      --restore )  DO_RESTORE=1
                   shift 1
                   ;;
      --merge )    DO_MERGE=1
                   shift 1
                   ;;
      -- )         shift
                   break
                   ;;
      * )          echo "Invalid parameters: $1"
                   exit 1
                   ;;
   esac
done

main "$@"
