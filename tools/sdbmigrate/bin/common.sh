#!/bin/bash

TOOL_PATH=$(cd `dirname $0`; pwd)

DEFAULT_INSTALL_PATH="/etc/default/sequoiadb"
DEFAULT_HOSTNAME="localhost"
DEFAULT_SVCNAME="11810"
DEFAULT_USER=""
DEFAULT_PASSWORD=""
REPLACE_STR="@SDB@"

CL_SECTION="collections"
CS_SECTION="collectionspaces"
CL_BASIC_SECTION="collection"
CS_BASIC_SECTION="collectionspace"
ITEM_NUM="number"
SDB_SHELL="sdb"
CONN_STR="var db = new Sdb();"
CONN_ADD="$DEFAULT_HOSTNAME:$DEFAULT_SVCNAME"

FILE_MAX_SIZE=5120 # 5MB

function findTool()
{
    local tool_name=$1
    local tool_path=""
    if [ "$tool_name" = "" ]; then
        echo "[ERROR] The tool name is not specified!"
        exit 1
    fi

    #find in bin dir
    tool_path="$TOOL_PATH/../../../bin/$tool_name"
    if [ -f "$tool_path" ] ; then
        echo "$tool_path"
        return 0
    fi

    #find sdb install path
    if [ -f "${DEFAULT_INSTALL_PATH}" ] ; then
        source "$DEFAULT_INSTALL_PATH"
        if [ -f "${INSTALL_DIR}/bin/$tool_name" ] ; then
           tool_path="${INSTALL_DIR}/bin/$tool_name"
           echo "$tool_path"
           return 0
        fi
    fi
    echo "[ERROR] Can't find $tool_name!"
    exit 1
}
sdb=$(findTool $SDB_SHELL)

function version()
{
    if [ "$1" = "" ]; then
        echo "[ERROR] The tool is not specified!"
        exit 1
    fi
    $1 --version
    exit 0
}

# params: option value option value
# eg: --csname foo --cscl foo
function paramConflictCheck()
{
    if [ $# -ge 4 -a "$2" != "" -a "$4" != "" ]; then
        echo "[ERROR] "$1" and "$3" cannot be used at the same time"
        exit 1
    fi
}

# param: option value1 value2
# return option value1
function appendParam()
{
    if [ "$2" != "" ]; then
        echo "$1 $2"
    elif [ "$3" != "" ]; then
        echo "$1 $3"
    fi
}

function readInIfile()
{
    local ini_file=$1
    local section=$2
    local option=$3
    local section_value
    local ini_options=()
    local file_value=""
    local section_num=0
    if [ "$ini_file" = "" ];then
        echo "[ERROR] ini file name cannot be empty!"
        return 1
    fi
    if [ ! -f "$ini_file" ]; then
        echo "[ERROR] No such file: $ini_file"
        return 1
    fi

    file_value=`sed -e '/^#/d' ${ini_file}`
    if [ "$section" = "" ];then
        echo "[ERROR] Section cannot be empty!"
        return 1
    fi
    section_num=`echo "$file_value" | grep "\[$section\]" | wc -l`
    if [ $section_num -gt 1 ]; then
        echo "[ERROR] Repeat section: $section!"
        return 1
    fi

    if [ "${option}" = "" ];then
        section_value=$(echo "$file_value" | awk "/\[${section}\]/{a=1}a==1" | sed -e'1d' -e '/^$/d' -e 's/[ \t]*$//g' -e 's/^[ \t]*//g' -e 's/[ ]/@SDB@/g' -e '/\[/,$d' )
        ini_options=(${section_value})
        echo ${ini_options[@]}
    elif [ "${section}" != "" ] && [ "${option}" != "" ];then
        item_value=$(echo "$file_value" | awk -F '=' "/\[${section}\]/{a=1}a==1" | sed -e '1d' -e '/^$/d' -e '/^\[.*\]/,$d' -e "/^${option}.*=.*/!d" -e "s/^${option}.*= *//")
        echo "${item_value}"
    fi
    return 0
}

# foo.bar to --csname foo --clanme bar
function genCSCLParam()
{
    local cl_name
    local cs_name
    local array=(${1//./ })
    cs_name=${array[0]}
    cl_name=${array[1]}
    if [ "$cs_name" != "" ] && [ "$cl_name" != "" ] && [ ${#array[@]} = 2 ]; then
        echo "--csname ${array[0]} --clname ${array[1]}"
        return 0
    fi
    return 1
}

# localhost:11810 to --hostname localhost --svcname 11810
function genHostsParam()
{
    local hostname
    local svcname
    array=(${1//:/ })
    hostname=${array[0]}
    svcname=${array[1]}
    if [ "$hostname" != "" ] && [ "$svcname" != "" ] && [ ${#array[@]} = 2 ]; then
        echo "--hostname $hostname --svcname $svcname"
        return 0
    fi
    return 1
}

function contains()
{
    local item=$1
    local arr=$2
    if [ "$item" = "" ]; then
        return 1
    fi
    for var in ${arr[@]}; do
        if [ "$var" = "$item" ]; then
            return 0
        fi
    done
    return 1
}

function setConnectParam()
{
    local hostname=$1
    local svcname=$2
    local username=$3
    local password=$4
    local token=$5
    local cipherfile=$6

    if [ "" = "$hostname" ] ; then
        hostname="$DEFAULT_HOSTNAME"
    fi
    if [ "" = "$svcname" ] ; then
        svcname="$DEFAULT_SVCNAME"
    fi
    if [ "" = "$username" ] ; then
        username="$DEFAULT_USER"
    fi
    if [ "" = "$password" ] ; then
        password="$DEFAULT_PASSWORD"
    fi

    if [ "$cipherfile" = "" ]; then
        CONN_STR="var db = new Sdb('$hostname',$svcname,'$username','$password');"
    else
        CONN_STR="var user = new CipherUser('$username').token('$token').cipherFile('$cipherfile'); var db = new Sdb('$hostname',$svcname,user);"
    fi
    CONN_ADD="$hostname:$svcname"
}

function getCLList()
{
    local cs_name=$1
    local rc=0

    if [ "$cs_name" != "" ]; then
        get_cl_str="var cursor = db.snapshot(SDB_SNAP_COLLECTIONSPACES, {Name:'$cs_name'});
                    while(cursor.next()){
                        var obj = cursor.current().toObj().Collection;
                        for(var i=0;i<obj.length;i++){
                            println(obj[i].Name)
                        }
                    }
                    cursor.close();"
    else
        get_cl_str="var cursor = db.snapshot(SDB_SNAP_COLLECTIONSPACES);
                    while(cursor.next()){
                        var obj = cursor.current().toObj().Collection;
                        for(var i=0;i<obj.length;i++){
                            println(obj[i].Name)
                        }
                    }
                    cursor.close();"
    fi

    cl_list=`$sdb -s "$CONN_STR $get_cl_str db.close();"`
    rc=$?
    echo "${cl_list[@]}"
    return $rc
}

function getNodeList()
{
    local cl_name=$1
    local rc=0

    if [ "" = "$cl_name" ]; then
        echo "Collections name must be specified"
        return 1
    fi

    get_node_str="var nodeArr = new Array();
                  var cursor = db.snapshot(SDB_SNAP_HEALTH);
                  if (cursor.size() < 2){
                      var clInfoArr = db.snapshot(SDB_SNAP_COLLECTIONS,{Name: '$cl_name'});
                      if (clInfoArr.size() > 0){
                          nodeArr.push('$CONN_ADD');
                      }
                      clInfoArr.close();
                  } else{
                      var clInfoArr = db.snapshot(SDB_SNAP_CATALOG,{Name: '$cl_name'});
                      while(clInfoArr.next()){
                          var obj = clInfoArr.current().toObj().CataInfo;
                          var isMainCL = clInfoArr.current().toObj().IsMainCL;
                          if (isMainCL == true)
                          {
                              nodeArr.push('$CONN_ADD');
                              break;
                          }
                          for(var i=0;i<obj.length;i++){
                              var rg = db.getRG(obj[i].GroupName);
                              var rgInfo = rg.getDetail().current().toObj().Group;
                              if ( rgInfo.length > 0 )
                              {
                                  var host = rgInfo[0].HostName;
                                  var svcname = rgInfo[0].Service[0].Name;
                                  nodeArr.push(host+':'+svcname);
                              }
                          }
                      }
                      clInfoArr.close();
                  }
                  cursor.close();
                  for (var i=0; i<nodeArr.length; i++){
                      if (nodeArr.indexOf(nodeArr[i]) == i){
                          println(nodeArr[i]);
                      }
                  }"

    node_list=`$sdb -s "$CONN_STR $get_node_str db.close();"`
    rc=$?
    echo "${node_list[@]}"
    return $rc
}

function arrToStr()
{
    local str=""
    for var in $@; do
        str="$str $var"
    done
    if [ ${#str} -gt 0 ]; then
        echo "${str:1:${#str}-1}"
    else
        echo ""
    fi
}

function jionPath()
{
    local dir=$1
    local file=$2
    last=${dir:${#dir}-1:${#dir}}
    if [ "$last" = "/" -o "$dir" = "" -o "$file" = "" ]; then
        echo "$dir$file"
    else
        echo "$dir/$file"
    fi
}

function getCLInfo()
{
    local csnaem
    local clname

    csname=$(getOptionFCmd "--csname" $@)
    clname=$(getOptionFCmd "--clname" $@)

    if [ "$csname" = "" -o "$clname" = "" ]; then
        echo ""
    else
        echo "$csname.$clname"
    fi
}

function getHostInfo()
{
    local hostname
    local svcname

    hostname=$(getOptionFCmd "--hostname" $@)
    svcname=$(getOptionFCmd "--svcname" $@)
    if [ "$hostname" = "" -o "$svcname" = ""  ]; then
        echo ""
    else
        echo "Node: $hostname:$svcname"
    fi
}

function getOptionFCmd()
{
    local opt=$1
    local value=""
    shift
    while [ "$1" != "" ]; do
        case $1 in
            "$opt" )   value=$2;  shift;;
        esac
        shift
    done
    echo "$value"
}

function dealQuotes()
{
    local opt=$1
    local value=$2
    local first_str="${value:0:1}"

    if [ "$value" = "" ]; then
        return 0
    fi
    case $opt in
        *delrecord* | *delchar* | *delfield* | *floatfmt* |  *sort* | *select* | *filter* | *datefmt* | *timestampfmt* | *fields* )

         if [ "$first_str" != "'" ]; then
             value="'$2'"
         fi
         ;;
    esac
    echo "$value"
}

function genResultFile()
{
    local file=$1
    local file_sz=0
    local msg
    if [ "$file" = "" ]; then
        echo "[ERROR] Need to specify the generated file name"
        exit 1
    fi
    if [ ! -f "$file" ]; then
        msg=$(touch "$file" 2>&1)
        if [ $? != 0 ]; then
            echo "[ERROR] $msg"
            exit 1
        fi
    fi
    file_sz=$(du -s "$file" | awk '{print $1}')
    if [ $file_sz -ge $FILE_MAX_SIZE ]; then
        echo "[REMIND] This file '$file' is large, it is recommended to clean up"
    fi
}

function repeatCheck()
{
    if [ $# -eq 2 -a "$2" != "" ]; then
        echo "[ERROR] Repeated options '$1'"
        exit 1
    fi

}

function paresFields()
{
    local result
    local cl_full_name=$1
    local first_str
    shift
    for var in $@; do
        first_str="${var:0:1}"
        if [ "$first_str" = "'" ]; then
            var="${var:1:${#var}}"
        fi
        arr=(${var//:/ })
        if [ ${#arr[@]} == 1 ]; then
            result="${var//$REPLACE_STR/ }"
            continue
        fi

        cl_full_name_tmp=${arr[0]}
        if [ "$cl_full_name" == "$cl_full_name_tmp" ]; then
           result="${arr[1]//$REPLACE_STR/ }"
        fi
    done
    if [ "$first_str" = "'" -a "$result" != "" ]; then
        echo "'$result"
    else
        echo "$result"
    fi
}

function countTimeD()
{
    local result=""
    local start_time=$1
    local end_time=$2
    if [ "$start_time" = "" -o "$end_time" = "" ]; then
        echo $result
        return 0
    fi
    local start_tmsp=$(date --date="$start_time" +%s)
    local end_tmsp=$(date --date="$end_time" +%s)
    local diff_tmsp=$((end_tmsp-start_tmsp))
    local hours=$((diff_tmsp/3600))
    local minutes=$((diff_tmsp/60%60))
    local seconds=$((diff_tmsp%60))
    if [ $hours -gt 0 ]; then
        result="${hours}h"
    fi
    if [ $minutes -gt 0 ]; then
        result="${result}${minutes}m"
    fi
    result="${result}${seconds}s"
    echo "$result"
}