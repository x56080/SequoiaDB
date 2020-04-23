#!/bin/bash

#Get file version
#$1 file name
#$2 filename of fuzzy matching( excluding extension )
#$3 extension
function getVersion()
{
   local version=${1#"$2-"}
   version=${version%".$3"}
   version=${version%%-*}
   echo $version;
}

#Get file version
#$1 file name
#$2 extension
function getSvn()
{
   local version=${1##*-r}
   version=${version%".$2"}

   if [ -n "$version" -a "$version" = "${version//[^0-9]/}" ]; then
      echo "$version" ;
   else
      echo "0";
   fi
}

function getMainVer()
{
   local version=${1%%.*}
   echo $version;
}

function getSubVer()
{
   local num=`echo $1 | grep -o "\." | wc -l`
   local version=${1#*.}

   if [ $num -ge 1 ]; then
      version=${version%.*}
      echo $version;
   else
      echo "0";
   fi
}

function getFixVer()
{
   local num=`echo $1 | grep -o "\." | wc -l`
   local version=${1##*.}

   if [ $num -eq 2 ]; then
      echo $version;
   else
      echo "0";
   fi
}

function compareVersion()
{
   local result="0"
   if [ $1 -gt $4 ]; then
      result="1";
   elif [ $1 -eq $4 ]; then
      if [ $2 -gt $5 ]; then
          result="1";
      elif [ $2 -eq $5 ]; then
         if [ $3 -gt $6 ]; then
            result="1";
         elif [ $3 -eq $6 ]; then
            if [ $7 -gt $8 ]; then
               result="1";
            fi
         fi
      fi
   fi
   echo $result;
}

#Get the file name of the executable file
#$1 find exec file path
#$2 filename of fuzzy matching( excluding extension )
#$3 extension
function getExecFileName()
{
   local fileList=`ls $1`;
   local result=""
   local v1=3
   local v2=0
   local v3=0
   local svn=0

   for fileName in $fileList;
   do
      # it is a file
      if [ -f "$1/$fileName" ]; then
         if [ "$3"x == ""x -a "${fileName%.*}"x == "${fileName##*.}"x ]; then
            # no ext name
            # matching file name
            if [[ "${fileName%.*}" == $2* ]]; then
               local version=`getVersion $fileName "$2" "$3"`;
               local tv1=`getMainVer "$version"`
               local tv2=`getSubVer "$version"`
               local tv3=`getFixVer "$version"`
               local tsvn=`getSvn $fileName "$3"`
               local rv=`compareVersion "$tv1" "$tv2" "$tv3" "$v1" "$v2" "$v3" "$tsvn" "$svn"`

               if [ "$rv"x == "1x" ]; then
                  result="$fileName"
                  v1="$tv1"
                  v2="$tv2"
                  v3="$tv3"
                  svn="$tsvn"
               fi
            fi
         else
            #matching file ext name
            if [ "${fileName##*.}"x = "$3"x ]; then
               # matching file name
               if [[ "${fileName%.*}" == $2* ]]; then
                  local version=`getVersion $fileName "$2" "$3"`;
                  local tv1=`getMainVer "$version"`
                  local tv2=`getSubVer "$version"`
                  local tv3=`getFixVer "$version"`
                  local tsvn=`getSvn $fileName "$3"`
                  local rv=`compareVersion "$tv1" "$tv2" "$tv3" "$v1" "$v2" "$v3" "$tsvn" "$svn"`

                  if [ "$rv"x == "1x" ]; then
                     result="$fileName"
                     v1="$tv1"
                     v2="$tv2"
                     v3="$tv3"
                     svn="$tsvn"
                  fi
               fi
            fi
         fi
      fi
   done

   echo "$result";
}

#Get the process id
#$1 filename
#$2 startup args
function getProcId()
{
   echo $(ps -efww|grep "$1"|grep " $2$\| $2 "|grep -v grep|awk '{print $2}');
}

#Get the process id
#$1 filename
#$2 startup args
function getProcIdTest()
{
   local pid=""
   local procList=$(ps -efww|grep "$1"|grep -v grep);

   IFS_old=$IFS
   IFS=$'\n'

   #echo $procList
   for proc in $procList;
   do
      IFS=$IFS_old
      local i=0

      for info in $proc;
      do
         let i=i+1

         if [ $i -eq 2 ] ; then
            tmp=$info
         fi

         if [ $i -ge 9 -a "$info"x == "$2"x ] ; then
            pid=$tmp
            echo $pid
            break
         fi
      done

      IFS=$'\n'

      if [ "$pid"x != ""x ] ; then
         break;
      fi
   done
   IFS=$IFS_old
}

#Get the common config of the plugin
#$1 shell file path
#$2 key
function getConfig()
{
   local configFile="$1/../../plugin.conf"
   local context=`cat $configFile`
   if [ $? == 0 ]; then
      context=`cat $configFile | grep "$2" | awk -F'=' '{ print $2 }' | sed s/[[:space:]]//g`
      echo "$context";
   fi
}

#startup plugin proc
#$1 works path
#$2 exec file path
#$3 httpname
#$4 other args
function startupPlugin()
{
   cd $1

   nohup $2 --__omhttpname=$3 $4 >>plugin.log 2>&1 &
}
