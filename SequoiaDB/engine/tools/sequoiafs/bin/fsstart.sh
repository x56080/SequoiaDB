#!/bin/bash
BashPath=$(dirname $(readlink -f $0))

pwdpath=$(pwd)

function Usage()
{
    echo  "Usage: fsstart [options] [args]"
    echo  "Command options:"
    echo  "  -h [ --help ]             help information"
    echo  "  -a [ --all ]              mount all sequoiafs with config in conf dir"
    echo  "  -m [ --mountpoint ] arg   mount the specified mountpoint"
    echo  "  --alias arg               mount the specified mountpoint with alias "
    echo  "  -c [ --confpath ] arg     mount the specified mountpoint with config file in specified conf path"
}

function Start()
{
  if [ -f "/opt/sequoiadb/fuse/bin/fusermount" ]; then
    PATH=/opt/sequoiadb/fuse/bin:$PATH
  fi 

  fsbin="$BashPath/sequoiafs"
  confrootpath="$BashPath/../conf/local"
  logrootpath="$BashPath/../log"
  if [ -d "$confrootpath" ]; then
    confrootpath=$(cd "$BashPath/../conf/local"; pwd)
  fi
  if [ -d "$logrootpath" ]; then
    logrootpath=$(cd "$BashPath/../log"; pwd)
  fi    
  
  startall=$1
  mountpointarg=$2
  aliasarg=$3
  confpatharg=$4
  logpath=$5
  otherargs=$6

  nessargs=()
  nesscount=0
  
  successcount=0
  failcount=0
  
  if [ -z "$startall" ] && [ -z "$mountpointarg" ] && [ -z "$aliasarg" ] && [ -z "$confpatharg" ]; then
    echo -e "fsstart.sh requires at least one parameter."
    Usage
    exit 127
  fi    
  
  if [ -n "$startall" ] ; then
    if [ ! -d "$confrootpath" ]; then
      echo "confpath($confrootpath) does not exist."
      exit 8
    fi 
    for dir in `ls "$confrootpath"`
    do
      nessargs=()
      curfspid=""
      confpatharg="$confrootpath/$dir"
      if [ -f "$confpatharg/sequoiafs.conf" ]; then
        nessargs[$nesscount]="-c"
        let "nesscount++"
        nessargs[$nesscount]="$confpatharg"
        let "nesscount++"
        source "$confpatharg/sequoiafs.conf"
        if [ -z $diagpath ]; then
          nessargs[$nesscount]="--diagpath"
          let "nesscount++"
          nessargs[$nesscount]="$logrootpath/$dir"
          let "nesscount++"
          logpath="$logrootpath/$dir"
        else
          logpath=$diagpath    
        fi
        echo "$fsbin ${nessargs[*]} ${otherargs[*]}"
        $fsbin ${nessargs[*]} ${otherargs[*]} &
        curfspid=$(jobs -l| grep "+" | grep -v grep | awk '{print $2}')
        
        loop=0
        while(( $loop < 100 ))
        do
          let "loop++"
          if [ -f "$logpath/sequoiafs.pid" ]; then
            fslogpid=$(cat "$logpath/sequoiafs.pid")
            if [ "$fslogpid" == "$curfspid" ]; then
              break
            else
              if [ -n "$( ps -ef |grep $curfspid |grep -v grep)" ]; then
                sleep 1
                continue
              else
                break
              fi
            fi
            break
          else
            if [ -n "$( ps -ef |grep $curfspid |grep -v grep)" ]; then
              sleep 1
              continue
            else
              break
            fi              
          fi
        done
        
        sleep 1
        
        if [ -f "$logpath/sequoiafs.pid" ]; then
          fslogpid=$(cat "$logpath/sequoiafs.pid")
          if [ "$fslogpid" == "$curfspid" ]; then
            echo "DONE"
            let "successcount++"
          else
            echo "FAILED"
            let "failcount++"    
          fi
        else
          echo "FAILED"
          let "failcount++"    
        fi
      else
        echo "confpath:$confpatharg is empty."
        let "failcount++"          
      fi    
    done
  else
    if [ -z "$confpatharg" ]; then
      if [ ! -d "$confrootpath" ]; then
        echo "confpath($confrootpath) does not exist."
        exit 8
      fi 
      if [ "$aliasarg" != "" ]; then
        confpath="$confrootpath/$aliasarg"
        if [ ! -d "$confpath" ]; then
          echo "can not find confpath"
          exit 127
        fi
        confpatharg=$confpath
      else
        mountpointarg=$(echo ${mountpointarg%*/})
        for dir in `ls "$confrootpath"`
        do
          tmpconfpath="$confrootpath/$dir"
          if [ -f "$tmpconfpath/sequoiafs.conf" ]; then  
            source "$tmpconfpath/sequoiafs.conf"
            mountcfginfo=$(echo ${mountpoint%*/})
            if [ "$mountpointarg" == "$mountcfginfo" ]; then
              confpatharg=$tmpconfpath;
              break
            fi
          fi        
        done  
        if [ -z "$confpatharg" ]; then
          echo "can not find confpath"
          exit 127
        fi
      fi  
    fi
    
    if [ "$confpatharg" != "" ]; then
      nessargs[$nesscount]="-c"
      let "nesscount++"
      nessargs[$nesscount]="$confpatharg"
      let "nesscount++"    
    fi
    
    if [ "$mountpointarg" != "" ]; then
      nessargs[$nesscount]="-m"
      let "nesscount++"
      nessargs[$nesscount]="$mountpointarg"    
      let "nesscount++"
    fi
    if [ "$aliasarg" != "" ]; then
      nessargs[$nesscount]="--alias"
      let "nesscount++"
      nessargs[$nesscount]=$aliasarg
      let "nesscount++"
    fi
    
    if [ "$logpath" != "" ]; then
      nessargs[$nesscount]="--diagpath"
      let "nesscount++"
      nessargs[$nesscount]=$logpath
      let "nesscount++"
    else
      diagpath=""
      if [ -f "$confpatharg/sequoiafs.conf" ]; then
        source "$confpatharg/sequoiafs.conf"
      fi
      if [ -z $diagpath ]; then
        lastdir=`echo $confpatharg | sed "s/\// /g" | awk 'NR==1{print $NF}' `      
        logpath="$logrootpath/$lastdir"    
        nessargs[$nesscount]="--diagpath"
        let "nesscount++"
        nessargs[$nesscount]=$logpath
        let "nesscount++"
      else 
        logpath=$diagpath      
      fi
    fi

    echo "$fsbin ${nessargs[*]} ${otherargs[*]} "
    $fsbin ${nessargs[*]} ${otherargs[*]} & 
    curfspid=$(jobs -l| grep "+" | grep -v grep | awk '{print $2}')
    
    loop=0
    while(( $loop < 100 ))
    do
      let "loop++"
      if [ -f "$logpath/sequoiafs.pid" ]; then
        fslogpid=$(cat "$logpath/sequoiafs.pid")
        if [ "$fslogpid" == "$curfspid" ]; then
          break
        else
          if [ -n "$( ps -ef |grep $curfspid |grep -v grep)" ]; then
            sleep 1
            continue
          else
            break
          fi
        fi
      else
        if [ -n "$( ps -ef |grep $curfspid |grep -v grep)" ]; then
          sleep 1
          continue
        else
          break
        fi              
      fi
    done
    
    sleep 1 
    
    if [ -f "$logpath/sequoiafs.pid" ]; then
      fslogpid=$(cat "$logpath/sequoiafs.pid")
      if [ "$fslogpid" == "$curfspid" ]; then
        echo "DONE"
        let "successcount++"
      else
        echo "FAILED"
        let "failcount++"    
      fi
    else
      echo "FAILED"
      let "failcount++"
    fi
  fi
  
  total=$(($successcount+$failcount))
  echo "Total: $total; Succeed: $successcount; Failed: $failcount"
  if [[ $failcount > 0 && $successcount > 0 ]]; then
    exit 2
  fi
  if [[ $failcount > 0 && $successcount == 0 ]]; then
    exit 4
  fi

  return 0
}

all=""
mountpoint=""
alias=""
logpath=""
configpath=""
otherargs=""
count=0

while [ -n "$1" ]
do
    case $1 in
    -h|--help)
      Usage
      exit 0
      ;;
    -a|--all)
      all="true"
      ;;
    -m|--mountpoint)
      mountpoint=$2
      shift
      ;;
    --alias)
      alias=$2
      shift
      ;;
    -c|--confpath)
      configpath=$2
      shift
      ;;
    --diagpath)
      logpath=$2
      shift
      ;;      
    *)
      otherargs[$count]=$1
      let count++
      otherargs[$count]=$2
      let count++
      shift
      ;;
    esac
shift
done

Start "$all" "$mountpoint" "$alias" "$configpath" "$logpath" ${otherargs[*]}

exit 0
