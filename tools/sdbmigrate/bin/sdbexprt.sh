#!/bin/bash

CUR_PATH=$(pwd)
TOOL_PATH=$(cd `dirname $0`; pwd)
# import common functions
source ${TOOL_PATH}/common.sh

export_result_file="${CUR_PATH}/sdbexprt.result"
export_tmp_file="${CUR_PATH}/$$.tmp"
export_tool="sdbexprt"
TMP_FIFO_FILE_1="${CUR_PATH}/$$.1.fifo"
TMP_FIFO_FILE_2="${CUR_PATH}/$$.2.fifo"

# result file info
total_num=0
success_num=0
faile_num=0
start_time=""
end_time=""

opt_list=("-h" "--help" "-V" "--version" "--debug" "-s" "--hostname" "-p" "--svcname" "--hosts" 
"-u" "--user" "-w" "--password" "--cipher" "--token" "--cipherfile" "-r" "--delrecord" "--filelimit" 
"--type" "--withid" "--fields" "--ssl" "--floatfmt" "--replace" "-c" "--csname" "-l" "--clname" 
"--select" "--filter" "--sort" "--skip" "--limit" "--cscl" "--excludecscl" "--strict" "-a" "--delchar" 
"-e" "--delfield" "--included" "--includebinary" "--includeregex" "--force" "--kicknull" "--checkdelimeter" 
"--conf" "--dir" "--jobs" "--file")

opt_hostname=""
opt_svcname=""
opt_user=""
opt_password=""
opt_cipher=""
opt_token=""
opt_cipherfile=""
opt_delrecord=""
opt_filelimit=""
opt_type=""
opt_withid=""
opt_fields=""
opt_ssl=""
opt_floatfmt=""
opt_replace=""
opt_csname=""
opt_clname=""
opt_select=""
opt_filter=""
opt_sort=""
opt_file=""
opt_skip=""
opt_limit=""
opt_excludecscl=""
opt_dir=""
opt_strict=""
opt_delchar=""
opt_delfield=""
opt_included=""
opt_includebinary=""
opt_includeregex=""
opt_force=""
opt_kicknull=""
opt_checkdelimeter=""
opt_conf=""
opt_jobs=""
opt_debug=""

#general params
opt_general_params=""

export_cmd_list=()
cl_params_list=()

function getParamValue()
{
    local value
    if [ $# != 2 ]; then
        return 0
    fi
    value=$(dealQuotes $1 "$2")
    case $1 in
        -s | --hostname     ) repeatCheck "$1" "$opt_hostname";       opt_hostname=$value       ;;
        -p | --svcname      ) repeatCheck "$1" "$opt_svcname";        opt_svcname=$value        ;;
        --hosts             ) repeatCheck "$1" "$opt_hostname:$opt_svcname";
                              opt_hostname=${value%%:*};              opt_svcname=${value##*:}  ;;
        -u | --user         ) repeatCheck "$1" "$opt_user";           opt_user=$value           ;;
        -w | --password     ) repeatCheck "$1" "$opt_password";       opt_password=$value       ;;
        --cipher            ) repeatCheck "$1" "$opt_cipher";         opt_cipher=$value         ;;
        --token             ) repeatCheck "$1" "$opt_token";          opt_token=$value          ;;
        --cipherfile        ) repeatCheck "$1" "$opt_cipherfile";     opt_cipherfile=$value     ;;
        -r | --delrecord    ) repeatCheck "$1" "$opt_delrecord";      opt_delrecord=$value      ;;
        --filelimit         ) repeatCheck "$1" "$opt_filelimit";      opt_filelimit=$value      ;;
        --type              ) repeatCheck "$1" "$opt_type";           opt_type=$value           ;;
        --withid            ) repeatCheck "$1" "$opt_withid";         opt_withid=$value         ;;
        --fields            ) repeatCheck "$1" "$opt_fields";         opt_fields=$value         ;;
        --floatfmt          ) repeatCheck "$1" "$opt_floatfmt";       opt_floatfmt=$value       ;;
        -c | --csname       ) repeatCheck "$1" "$opt_csname";         opt_csname=$value         ;;
        -l | --clname       ) repeatCheck "$1" "$opt_clname";         opt_clname=$value         ;;
        --select            ) repeatCheck "$1" "$opt_select";         opt_select=$value         ;;
        --filter            ) repeatCheck "$1" "$opt_filter";         opt_filter=$value         ;;
        --sort              ) repeatCheck "$1" "$opt_sort";           opt_sort=$value           ;;
        --file              ) repeatCheck "$1" "$opt_file";           opt_file=$value           ;;
        --skip              ) repeatCheck "$1" "$opt_skip";           opt_skip=$value           ;;
        --limit             ) repeatCheck "$1" "$opt_limit";          opt_limit=$value          ;;
        --cscl              ) repeatCheck "$1" "$opt_cscl";           opt_cscl=$value           ;;
        --excludecscl       ) repeatCheck "$1" "$opt_excludecscl";    opt_excludecscl=$value    ;;
        --dir               ) repeatCheck "$1" "$opt_dir";            opt_dir=$value            ;;
        --strict            ) repeatCheck "$1" "$opt_strict";         opt_strict=$value         ;;
        -a | --delchar      ) repeatCheck "$1" "$opt_delchar";        opt_delchar=$value        ;;
        -e | --delfield     ) repeatCheck "$1" "$opt_delfield";       opt_delfield=$value       ;;
        --included          ) repeatCheck "$1" "$opt_included";       opt_included=$value       ;;
        --includebinary     ) repeatCheck "$1" "$opt_includebinary";  opt_includebinary=$value  ;;
        --includeregex      ) repeatCheck "$1" "$opt_includeregex";   opt_includeregex=$value   ;;
        --force             ) repeatCheck "$1" "$opt_force";          opt_force=$value          ;;
        --kicknull          ) repeatCheck "$1" "$opt_kicknull";       opt_kicknull=$value       ;;
        --checkdelimeter    ) repeatCheck "$1" "$opt_checkdelimeter"; opt_checkdelimeter=$value ;;
        --conf              ) repeatCheck "$1" "$opt_conf_file";      opt_conf_file=$value      ;;
        --jobs              ) repeatCheck "$1" "$opt_jobs";           opt_jobs=$value           ;;
        # general params, like ssl
        *                   ) opt_general_params="${opt_general_params} $1 $value"              ;;
    esac
}

function dealParamArr()
{
    local value=""
    local opt=""
    while [ "$1" != "" ]; do
        contains $1 "${opt_list[*]}"
        if [ "$?" = 0 ]; then
            opt=$1
        else
           shift
           continue
        fi
        case $opt in
            -h | --help    ) helpInfo                          ;;
            -V | --version ) version "$export_tool"            ;;
            --debug        ) opt_debug="true"; shift; continue ;;
        esac

        while [ "$2" != "" ]; do
            contains $2 "${opt_list[*]}"
            if [ $? != 0 ]; then
                value="$value$2 "
                shift
                continue
            elif [ "$value" = "" ]; then
                echo "[ERROR] Option $opt does not specify a value!"
                exit
            fi
            break
        done
        if [ "$value" != "" ]; then
            value=${value:0:${#value}-1}
            getParamValue $opt "$value"
            opt=""
            value=""
        else
            echo "[ERROR] Option $opt does not specify a value!"
            exit
        fi
        shift
    done
}

function parseParam()
{
    local params
    if [ $# = 0 ]; then
        helpInfo
    fi
    params=(${@//=/ })
    dealParamArr ${params[@]}
    checkParams
    setDefualtValue
}

function setDefualtValue()
{
    if [ "$opt_hostname" = "" ]; then
        opt_hostname="$DEFAULT_HOSTNAME"
    fi
    if [ "$opt_svcname" = "" ]; then
        opt_svcname="$DEFAULT_SVCNAME"
    fi
    if [ "$opt_type" = "" ]; then
        opt_type="csv"
    fi
}

function checkParams()
{
    paramConflictCheck "--conf"   "$opt_conf_file" "--csname"      "$opt_csname"
    paramConflictCheck "--conf"   "$opt_conf_file" "--clname"      "$opt_clname"
    paramConflictCheck "--conf"   "$opt_conf_file" "--select"      "$opt_select"
    paramConflictCheck "--conf"   "$opt_conf_file" "--filter"      "$opt_filter"
    paramConflictCheck "--conf"   "$opt_conf_file" "--sort"        "$opt_sort"
    paramConflictCheck "--conf"   "$opt_conf_file" "--file"        "$opt_file"
    paramConflictCheck "--conf"   "$opt_conf_file" "--skip"        "$opt_skip"
    paramConflictCheck "--conf"   "$opt_conf_file" "--limit"       "$opt_limit"
    paramConflictCheck "--conf"   "$opt_conf_file" "--cscl"        "$opt_cscl"
    paramConflictCheck "--conf"   "$opt_conf_file" "--excludecscl" "$opt_excludecscl"
    paramConflictCheck "--conf"   "$opt_conf_file" "--dir"         "$opt_dir"
    paramConflictCheck "--conf"   "$opt_conf_file" "--fields"      "$opt_fields"
    paramConflictCheck "--select" "$opt_select"    "--fields"      "$opt_fields"
    paramConflictCheck "--cscl"   "$opt_cscl"      "--csname"      "$opt_csname"
    paramConflictCheck "--cscl"   "$opt_cscl"      "--clname"      "$opt_clname"
    paramConflictCheck "--file"   "$opt_file"      "--dir"         "$opt_dir"
    paramConflictCheck "--file"   "$opt_file"      "--jobs"        "$opt_jobs"
}

function helpInfo()
{
    $export_tool "--help" | sed '/--dir /d' | sed '/--conf /d' | sed '/--genconf /d' | sed '/--genfields /,+2 d'
    echo "  --conf arg             The configuration file for the collection and "
    echo "                         collectionspace                               "
    echo "Output Options:                                                        "
    echo "  --dir arg              The directory where the data was exported     "
    echo "Other Options:                                                         "
    echo "  --jobs arg             The number of concurrent exports              "
    echo "  --compress arg         Whether to turn on compression, default false "
    exit 0
}

function parseGeneralParams()
{
    local opt
    local value
    local cl_params
    local cl_delrecord
    local cl_filelimit
    local cl_type
    local cl_withid
    local cl_fields
    local cl_floatfmt
    local cl_strict
    local cl_delchar
    local cl_delfield
    local cl_included
    local cl_includebinary
    local cl_includeregex
    local cl_force
    local cl_kicknull
    local cl_checkdelimeter
    local cl_compress
    while [ "$1" != "" ]; do
        opt=${1%%=*}
        value=${1#*=}
        if [ "$value" = "" ]; then
            shift
            continue
        fi
        value="${value//$REPLACE_STR/ }"
        value=$(dealQuotes $opt "$value")
        case $opt in
            delrecord      ) repeatCheck "$opt" "$cl_delrecord";      cl_delrecord=$value      ;;
            filelimit      ) repeatCheck "$opt" "$cl_filelimit";      cl_filelimit=$value      ;;
            withid         ) repeatCheck "$opt" "$cl_withid";         cl_withid=$value         ;;
            floatfmt       ) repeatCheck "$opt" "$cl_floatfmt";       cl_floatfmt=$value       ;;
            strict         ) repeatCheck "$opt" "$cl_strict";         cl_strict=$value         ;;
            delchar        ) repeatCheck "$opt" "$cl_delchar";        cl_delchar=$value        ;;
            delfield       ) repeatCheck "$opt" "$cl_delfield";       cl_delfield=$value       ;;
            included       ) repeatCheck "$opt" "$cl_included";       cl_included=$value       ;;
            includebinary  ) repeatCheck "$opt" "$cl_includebinary";  cl_includebinary=$value  ;;
            includeregex   ) repeatCheck "$opt" "$cl_includeregex";   cl_includeregex=$value   ;;
            force          ) repeatCheck "$opt" "$cl_force";          cl_force=$value          ;;
            kicknull       ) repeatCheck "$opt" "$cl_kicknull";       cl_kicknull=$value       ;;
            checkdelimeter ) repeatCheck "$opt" "$cl_checkdelimeter"; cl_checkdelimeter=$value ;;
        esac
        shift
    done
    cl_delrecord=$(appendParam       '--delrecord'      "$cl_delrecord"      "$opt_delrecord")
    cl_filelimit=$(appendParam       '--filelimit'      "$cl_filelimit"      "$opt_filelimit")
    cl_withid=$(appendParam          '--withid'         "$cl_withid"         "$opt_withid")
    cl_floatfmt=$(appendParam        '--floatfmt'       "$cl_floatfmt"       "$opt_floatfmt")
    cl_strict=$(appendParam          '--strict'         "$cl_strict"         "$opt_strict")
    cl_delchar=$(appendParam         '--delchar'        "$cl_delchar"        "$opt_delchar")
    cl_delfield=$(appendParam        '--delfield'       "$cl_delfield"       "$opt_delfield")
    cl_included=$(appendParam        '--included'       "$cl_included"       "$opt_included")
    cl_includebinary=$(appendParam   '--includebinary'  "$cl_includebinary"  "$opt_includebinary")
    cl_includeregex=$(appendParam    '--includeregex'   "$cl_includeregex"   "$opt_includeregex")
    cl_force=$(appendParam           '--force'          "$cl_force"          "$opt_force")
    cl_kicknull=$(appendParam        '--kicknull'       "$cl_kicknull"       "$opt_kicknull")
    cl_checkdelimeter=$(appendParam  '--checkdelimeter' "$cl_checkdelimeter" "$opt_checkdelimeter")

    cl_params=$(arrToStr "$cl_ssl" "$cl_replace" "$cl_delrecord" "$cl_filelimit" "$cl_withid" "$cl_floatfmt" "$cl_strict" "$cl_delchar" "$cl_delfield" "$cl_included" "$cl_includebinary" "$cl_includeregex" "$cl_force" "$cl_kicknull" "$cl_checkdelimeter")
    echo "$cl_params"
}

function buildCLParams()
{
    local cl_param=""
    local cl_node_list
    local cl_name
    local cl_full_name
    local cl_type
    local cl_select
    local cl_filter
    local cl_sort
    local cl_skip
    local cl_limit
    local cl_dir
    local cl_fields
    local cl_hosts
    local cl_compress
    local opt
    local value
    local i=0
    local cl_general_params
    local other_params
    local cl_type_param
    local cl_file_param
    local cl_params
    local msg
    while [ "$1" != "" ]; do
        opt=${1%%=*}
        value=${1#*=}
        if [ "$value" = "" ]; then
            shift
            continue
        fi
        value="${value//$REPLACE_STR/ }"
        value=$(dealQuotes $opt "$value")
        case $opt in
            name     ) repeatCheck "$opt" "$cl_name";    cl_name="$(genCSCLParam $value)";
                       cl_full_name=$value                                              ;;
            select   ) repeatCheck "$opt" "$cl_select";  cl_select=" --select $value"   ;;
            filter   ) repeatCheck "$opt" "$cl_filter";  cl_filter=" --filter $value"   ;;
            sort     ) repeatCheck "$opt" "$cl_sort";    cl_sort=" --sort $value"       ;;
            dir      ) repeatCheck "$opt" "$cl_dir";     cl_dir="$value"                ;;
            skip     ) repeatCheck "$opt" "$cl_skip";    cl_skip=" --skip $value"       ;;
            limit    ) repeatCheck "$opt" "$cl_limit";   cl_limit=" --limit $value"     ;;
            fields   ) repeatCheck "$opt" "$cl_fields";  cl_fields=" --fields $value"   ;;
            type     ) repeatCheck "$opt" "$cl_type";    cl_type=$value                 ;;
            *        ) other_params="$other_params $1"                                  ;;
        esac
        shift
    done

    if [ "$cl_full_name" = "" ]; then
        echo "[ERROR] Collection name must be specified"
        exit 1
    fi

    if [ "$cl_dir" = "" ]; then
        cl_dir="$CUR_PATH"
    fi

    cl_general_params=$(parseGeneralParams $other_params)
    if [ $? != 0 ]; then
        echo "[ERROR] $cl_general_params"
        exit 1
    fi
    if [ "$cl_type" = "" ]; then
        cl_type="$opt_type"
    fi

    if [ "$opt_jobs" = "" ]; then

        if [ -d "$cl_dir" ]; then
            cl_file=$(jionPath $cl_dir $cl_full_name.$cl_type)
        else
            cl_file="$cl_dir"
        fi

        cl_hostname=$(appendParam   '--hostname' "$opt_hostname")
        cl_svcname=$(appendParam    '--svcname'  "$opt_svcname")
        cl_file=$(appendParam       '--file'     "$cl_file")
        cl_type_param=$(appendParam '--type'     "$cl_type")

        cl_params=$(arrToStr $cl_hostname $cl_svcname $cl_name $cl_file $cl_type_param $cl_select $cl_filter $cl_sort $cl_skip $cl_limit $cl_fields "$cl_general_params")
        if [ "$cl_params" != "" ]; then
            cl_params_list[${#cl_params_list[@]}]="$cl_params"
        fi
    else
        # no support skip and limit
        paramConflictCheck "--jobs" "$opt_jobs" "--skip"  "$cl_skip"
        paramConflictCheck "--jobs" "$opt_jobs" "--limit" "$cl_limit"

        cl_node_list=$(getNodeList $cl_full_name)
        if [ $? -ne 0 ]; then
            echo "[ERROR] ${cl_node_list[@]}"
            exit 1
        fi

        if [ "${cl_node_list[@]}" = "" ]; then
            addResultBody "Collection '$cl_full_name' does not exist!\n"
            total_num=$[total_num+1]
            return 1
        fi
        for node in ${cl_node_list[@]}; do
            cl_type_param=""

            if [ "$cl_dir" != "" -a ! -d "$cl_dir" ]; then
                exec_cmd "mkdir -p $cl_dir 2>&1"
            fi
            cl_file=$(jionPath $cl_dir $cl_full_name.$i.$cl_type)
            i=$[i+1]
            cl_hosts=$(genHostsParam ${node})
            cl_file_param=$(appendParam '--file'     "$cl_file")
            cl_type_param=$(appendParam '--type'     "$cl_type")

            cl_params=$(arrToStr $cl_hosts $cl_name $cl_file_param $cl_type_param $cl_select $cl_filter $cl_sort $cl_fields "$cl_general_params")
            if [ "$cl_params" != "" ]; then
                cl_params_list[${#cl_params_list[@]}]="$cl_params"
            fi
        done
    fi
}

function buildCSParams()
{
    local opt
    local value
    local cs_name
    local excludecl
    local dir
    local cl_type
    local fields_list=()
    local cl_list_tmp
    local arr
    local cl_params
    local cl_name
    local cl_fields_param
    local cl_full_name
    local cl_node_list
    local cl_file
    local i=0
    local other_params
    local cs_name_param
    local cl_name_param
    local cl_file_param
    local cl_type_param
    local msg
    while [ "$1" != "" ]; do
        opt=${1%%=*}
        value=${1#*=}
        if [ "$value" = "" ]; then
            shift
            continue
        fi
        #value="${value//$REPLACE_STR/ }"
        value=$(dealQuotes $opt "$value")
        case $opt in
            name      ) repeatCheck "$opt" "$cs_name";     cs_name="${value//$REPLACE_STR/ }" ;;
            excludecl ) repeatCheck "$opt" "$excludecl";   value="${value//$REPLACE_STR/}";
                        excludecl=${value//,/ }                                               ;;
            dir       ) repeatCheck "$opt" "$dir";         dir="${value//$REPLACE_STR/ }"     ;;
            type      ) repeatCheck "$opt" "$cl_type";     cl_type="${value//$REPLACE_STR/ }" ;;
            fields    ) fields_list[${#fields_list[@]}]=$value                                ;;
            *         ) other_params="$other_params $1"                                       ;;
        esac
        shift
    done
    if [ "$cs_name" = "" ]; then
        echo "[ERROR] CollectionSpace name must be specified"
        exit 1
    fi

    if [ "$cl_dir" = "" ]; then
        cl_dir="$CUR_PATH"
    fi

    cl_general_params=$(parseGeneralParams $other_params)
    if [ $? != 0 ]; then
        echo "[ERROR] $cl_general_params"
        exit 1
    fi

    if [ "$cl_type" = "" ]; then
        cl_type="$opt_type"
    fi

    cl_list_tmp=$(getCLList $cs_name)
    if [ $? -ne 0 ]; then
        echo "[ERROR] ${cl_list_tmp[@]}"
        exit 1
    fi

    if [ "$cl_list_tmp" = "" ]; then
        addResultBody "CollectionSpace '$cs_name' does not exist or has no collection!\n"
        total_num=$[total_num+1]
        return 1
    fi
    for cl_full_name in ${cl_list_tmp[@]}; do
        arr=(${cl_full_name//./ })
        cl_name=${arr[1]}
        contains $cl_name "${excludecl[*]}"
        if [ $? -eq 0 ]; then
           continue
        fi
        cl_fields_param=$(paresFields $cl_full_name ${fields_list[@]})
        cl_node_list=$(getNodeList $cl_full_name)
        if [ $? -ne 0 ]; then
            echo "[ERROR] ${cl_node_list[@]}"
            exit 1
        fi

        if [ "$dir" != "" -a ! -d "$dir" ]; then
            exec_cmd "mkdir -p $dir 2>&1"
        fi

        if [ "$opt_jobs" = "" ]; then
            cl_file=$(jionPath $dir $cl_full_name.$cl_type)
            cl_hostname=$(appendParam   '--hostname' "$opt_hostname")
            cl_svcname=$(appendParam    '--svcname'  "$opt_svcname")
            cs_name_param=$(appendParam '--csname'   "$cs_name")
            cl_name_param=$(appendParam '--clname'   "$cl_name")
            cl_file_param=$(appendParam '--file'     "$cl_file")
            cl_type_param=$(appendParam '--type'     "$cl_type")
            cl_fields=$(appendParam     '--fields'   "$cl_fields_param")
            cl_params=$(arrToStr $cl_hostname $cl_svcname $cs_name_param $cl_name_param $cl_file_param $cl_type_param $cl_fields "$cl_general_params")
            if [ "$cl_params" != "" ]; then
                cl_params_list[${#cl_params_list[@]}]="$cl_params"
            fi
            continue
        fi

        i=0
        for cl_node in ${cl_node_list[@]}; do

            cl_file=$(jionPath $dir $cl_full_name.$i.$cl_type)
            i=$[i+1]

            cl_hosts=$(genHostsParam ${cl_node})
            cs_name_param=$(appendParam '--csname'   "$cs_name")
            cl_name_param=$(appendParam '--clname'   "$cl_name")
            cl_file_param=$(appendParam '--file'     "$cl_file")
            cl_type_param=$(appendParam '--type'     "$cl_type")
            cl_fields=$(appendParam     '--fields'   "$cl_fields_param")
            cl_params=$(arrToStr $cl_hosts $cs_name_param $cl_name_param $cl_file_param $cl_type_param $cl_fields "$cl_general_params")
            if [ "$cl_params" != "" ]; then
                cl_params_list[${#cl_params_list[@]}]="$cl_params"
            fi
        done
    done
}

function parseExportConf()
{
    local i
    local cl_sct
    local cs_sct
    local cl_param=()
    local cs_param=()
    local cl_num
    local cs_num
    local param
    
    if [ "$opt_conf_file" = "" ]; then
        return 0
    fi

    #parse cl param
    cl_num=$(readInIfile $opt_conf_file $CL_SECTION $ITEM_NUM)
    if [ "$cl_num" != "" ]; then
        for ((i=1;i<=$cl_num;i++));do
            cl_sct="${CL_BASIC_SECTION}$i"
            cl_param=$(readInIfile $opt_conf_file $cl_sct)
            if [ "$cl_param" = "" ]; then
                continue
            fi
            buildCLParams $cl_param
        done
    fi

    #parse cs param
    cs_num=$(readInIfile $opt_conf_file $CS_SECTION $ITEM_NUM)
    if [ "$cs_num" != "" ]; then
        for ((i=1;i<=$cs_num;i++));do
            cs_sct="${CS_BASIC_SECTION}$i"
            cs_param=$(readInIfile $opt_conf_file $cs_sct)
            if [ "$cs_param" = "" ]; then
                continue
            fi
            buildCSParams $cs_param
        done
    fi
}

function findEXCL()
{
    local cscl=$1
    local ex_arr=$2
    local cl_list=""
    local cl
    local cs
    arr=(${cscl//./ })
    cs=${arr[0]}
    cl=${arr[1]}
    for var in ${ex_arr//,/ } ; do
        arr=(${var//./ })
        cs_tmp=${arr[0]}
        cl_tmp=${arr[1]}
        if [ "$cs" != "$cs_tmp" ]; then
            continue
        fi
        if [ "$cl_tmp" = "" ]; then
            return 1
        fi
        if [ "$cl" = "" ]; then
            cl_list="$cl_list,$cl_tmp"
            continue
        fi

        if [ "$cl" != "$cl_tmp" ]; then
            continue
        else
            return 1
        fi
    done
    echo "$cl_list"
}

function parseCmdParam()
{
    local dir
    local cscl
    local arr
    local cs_name
    local cl_name
    local excl_list

    # cmd cl params
    if [ "$opt_csname" != "" -a "$opt_clname" != "" ]; then
        if [ "$opt_file" != "" ]; then
            dir=$opt_file
        else
            dir=$opt_dir
        fi
        buildCLParams "name=$opt_csname.$opt_clname" "dir=$dir" "select=$opt_select" "filter=$opt_filter" "sort=$opt_sort" "fields=$opt_fields" "skip=$opt_skip" "limit=$opt_limit"
    fi

    # cmd cscl params
    for cscl in ${opt_cscl//,/ }; do
        arr=(${cscl//./ })
        cs_name=${arr[0]}
        cl_name=${arr[1]}
        excl_list=$(findEXCL $cscl $opt_excludecscl)
        if [ $? != 0 ]; then
            continue
        fi
        if [ "$cl_name" != "" ]; then
            buildCLParams "name=$cscl" "dir=$opt_dir" "fields=$opt_fields"
        else
            buildCSParams "name=$cs_name" "excludecl=$excl_list" "dir=$opt_dir" "fields=$opt_fields"
        fi
    done
}

function buildExportCmd()
{
    local user=$(appendParam       '--user'       "$opt_user")
    local password=$(appendParam   '--password'   "$opt_password")
    local cipher=$(appendParam     '--cipher'     "$opt_cipher")
    local token=$(appendParam      '--token'      "$opt_token")
    local cipherfile=$(appendParam '--cipherfile' "$opt_cipherfile")

    opt_general_params=$(arrToStr $user $password $cipher $token $cipherfile $opt_general_params)

    setConnectParam "$opt_hostname" "$opt_svcname" "$opt_user" "$opt_password" "$opt_token" "$opt_cipherfile"

    #get cl params from cmd
    parseCmdParam

    #get cl params from conf file
    parseExportConf

    for ((i=0; i<${#cl_params_list[@]}; i++)); do
        export_cmd_list[i]="$export_tool ${cl_params_list[$i]} $opt_general_params"
    done
}

function genResultHead()
{
    local num=${#export_cmd_list[@]}
    total_num=$[total_num+num]
    success_num=$(sed -n '/Exported successfully with [1-9][0-9]* successful collections/p' "$export_tmp_file" | wc -l)
    faile_num=$[total_num-success_num]
    local consume_time=$(countTimeD "$start_time" "$end_time")
    local str_1="========== Export result =========="
    local str_2="Exported count   : $total_num"
    local str_3="Successful count : $success_num"
    local str_4="Failed count     : $faile_num"
    local str_5="Start time       : $start_time"
    local str_6="End time         : $end_time"
    local str_7="Consume time     : $consume_time"
    local str_8=""
    local str_9="---------- Export detail ----------"
    sed -i "1i$str_1\n$str_2\n$str_3\n$str_4\n$str_5\n$str_6\n$str_7\n$str_8\n$str_9" $export_tmp_file
}

function addResultBody()
{
    if [ $# -ne 0 ]; then
        local msg="$1"
        local export_cmd="$2"
        read -u7
        {
            echo -e "[Export]\n$msg" >> "$export_tmp_file"
            if [ "$export_cmd" != "" ]; then
                echo "$export_cmd" >> "$export_tmp_file"
                echo "" >> "$export_tmp_file"
            fi
            echo >&7
        }
    fi
}

function exec_cmd()
{
    local cmd="$@"
    local msg=""

    msg=$(eval "$cmd")
    if [ $? != 0 ]; then
        echo "[ERROR] $msg"
        exit 1
    fi
}

function exportData()
{
    local i
    local export_cmd
    local cl
    local hosts
    local msg
    # touch sdbexport.result
    genResultFile "$export_result_file"

    exec_cmd "touch $export_tmp_file"
    exec_cmd "mkfifo $TMP_FIFO_FILE_1 2>&1"
    exec_cmd "mkfifo $TMP_FIFO_FILE_2 2>&1"

    exec 6<>$TMP_FIFO_FILE_1
    exec 7<>$TMP_FIFO_FILE_2
    rm -f $TMP_FIFO_FILE_1
    rm -f $TMP_FIFO_FILE_2

    echo >&7
    buildExportCmd

    if [ "$opt_jobs" = "" ]; then
        opt_jobs=1
    fi
    for ((i=0; i<$opt_jobs; i++)); do
        echo >&6
    done

    for ((i=0; i<${#export_cmd_list[@]}; i++)); do
        export_cmd=${export_cmd_list[$i]}
        read -u6
        {
            cl=$(getCLInfo $export_cmd)
            hosts=$(getHostInfo $export_cmd)
            msg=$(eval "$export_cmd" 2>&1)
            if [ $? -ne 0 -o "$opt_debug" = "true" ]; then
                msg="$hosts\nCollection:$cl\n$msg"
                addResultBody "$msg" "Export cmd:$export_cmd"
            else
                msg="$hosts\nCollection:$cl\n$msg\n"
                addResultBody "$msg"
            fi
            msg=""
            echo >&6
        }&
    done
    wait
    exec 6>&-
    exec 7>&-
    end_time=$(date +'%Y-%m-%d %H:%M:%S')
    genResultHead

    cat $export_tmp_file >> $export_result_file
    rm -f $export_tmp_file

    echo "Exported finish, view the details from file $export_result_file"
}

function main()
{
    start_time=$(date +'%Y-%m-%d %H:%M:%S')
    export_tool=$(findTool $export_tool)
    if [ $? -ne 0 ]; then
        echo "$export_tool"
        return 1
    fi

    parseParam $@

    exportData
}

main $@
