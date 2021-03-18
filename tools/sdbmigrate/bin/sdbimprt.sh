#!/bin/bash

CUR_PATH=$(pwd)
TOOL_PATH=$(cd `dirname $0`; pwd)

# import common functions
source ${TOOL_PATH}/common.sh

import_result_file="${CUR_PATH}/sdbimprt.result"
import_tool="sdbimprt"

# result file info
total_num=0
success_num=0
faile_num=0
start_time=""
end_time=""
result_head=""
result_body=""

opt_list=("-h" "--help" "-V" "--version" "--debug" "-s" "--hostname" "-p" "--svcname" "--hosts" 
"-u" "--user" "-w" "--password" "--cipherfile" "--cipher" "--token" "-c" "--csname" "-l" 
"--clname" "--errorstop" "--ssl" "-v" "--verbose" "--file" "--exec" "--type" "--linepriority" 
"-r" "--delrecord" "--force" "--unicode" "--decimalto" "-a" "--delchar" "-e" "--delfield" 
"--fields" "--datefmt" "--timestampfmt" "--trim" "--headerline" "--sparse" "--extra" "--cast" 
"--strictfieldnum" "--checkdelimeter" "-n" "--insertnum" "-j" "--jobs" "--coord" "--sharding" 
"--transaction" "--allowkeydup" "--conf")

opt_fields=""
opt_csname=""
opt_clname=""
opt_file=""
opt_exec=""
opt_conf_file=""

opt_type=""
opt_linepriority=""
opt_delrecord=""
opt_force=""
opt_unicode=""
opt_decimalto=""
opt_delchar=""
opt_delfield=""
opt_datefmt=""
opt_timestampfmt=""
opt_trim=""
opt_headerline=""
opt_sparse=""
opt_extra=""
opt_cast=""
opt_strictfieldnum=""
opt_checkdelimeter=""
opt_debug=""
opt_verbose=""

#general params
opt_general_params=""

cl_params_list=()
import_cmd_list=()

function getParamValue()
{
    local value
    if [ $# != 2 ]; then
        return 0
    fi
    value=$(dealQuotes $1 "$2")
    case $1 in
        -c | --csname    ) repeatCheck "$1" "$opt_csname";         opt_csname=$value         ;;
        -l | --clname    ) repeatCheck "$1" "$opt_clname";         opt_clname=$value         ;;
        --file           ) repeatCheck "$1" "$opt_file";           opt_file=$value           ;;
        --exec           ) repeatCheck "$1" "$opt_exec";           opt_exec=$value           ;;
        --type           ) repeatCheck "$1" "$opt_type";           opt_type=$value           ;;
        --linepriority   ) repeatCheck "$1" "$opt_linepriority";   opt_linepriority=$value   ;;
        -r | --delrecord ) repeatCheck "$1" "$opt_delrecord";      opt_delrecord=$value      ;;
        --force          ) repeatCheck "$1" "$opt_force";          opt_force=$value          ;;
        --unicode        ) repeatCheck "$1" "$opt_unicode";        opt_unicode=$value        ;;
        --decimalto      ) repeatCheck "$1" "$opt_decimalto";      opt_decimalto=$value      ;;
        -a | --delchar   ) repeatCheck "$1" "$opt_delchar";        opt_delchar=$value        ;;
        -e | --delfield  ) repeatCheck "$1" "$opt_delfield";       opt_delfield=$value       ;;
        --fields         ) repeatCheck "$1" "$opt_fields";         opt_fields=$value         ;;
        --datefmt        ) repeatCheck "$1" "$opt_datefmt";        opt_datefmt=$value        ;;
        --timestampfmt   ) repeatCheck "$1" "$opt_timestampfmt";   opt_timestampfmt=$value   ;;
        --trim           ) repeatCheck "$1" "$opt_trim";           opt_trim=$value           ;;
        --headerline     ) repeatCheck "$1" "$opt_headerline";     opt_headerline=$value     ;;
        --sparse         ) repeatCheck "$1" "$opt_sparse";         opt_sparse=$value         ;;
        --extra          ) repeatCheck "$1" "$opt_extra";          opt_extra=$value          ;;
        --cast           ) repeatCheck "$1" "$opt_cast";           opt_cast=$value           ;;
        --strictfieldnum ) repeatCheck "$1" "$opt_strictfieldnum"; opt_strictfieldnum=$value ;;
        --checkdelimeter ) repeatCheck "$1" "$opt_checkdelimeter"; opt_checkdelimeter=$value ;;
        --conf           ) repeatCheck "$1" "$opt_conf";           opt_conf_file=$value      ;;
        *                ) opt_general_params="${opt_general_params} $1 $value"              ;;
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
            -h | --help    ) helpInfo                            ;;
            -V | --version ) version $import_tool                ;;
            --debug        ) opt_debug="true"; shift; continue   ;;
            -v | --verbose ) opt_verbose="true"; shift; continue ;;
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
}

function checkParams()
{
    paramConflictCheck "--conf" "$opt_conf_file" "--csname" "$opt_csname"
    paramConflictCheck "--conf" "$opt_conf_file" "--clname" "$opt_clname"
    paramConflictCheck "--conf" "$opt_conf_file" "--file"   "$opt_file"
    paramConflictCheck "--conf" "$opt_conf_file" "--exec"   "$opt_exec"
    paramConflictCheck "--conf" "$opt_conf_file" "--fields" "$opt_fields"
}

function helpInfo()
{
    $import_tool --help
    echo "CS/CL conf Options:                                                     "
    echo "  --conf arg             The configuration file for the collection and  "
    echo "                         collectionspace, cannot be used in conjunction "
    echo "                         with --csname,--clname                         "
    exit 0
}

function parseGeneralParams()
{
    local opt
    local value
    local cl_type
    local cl_linepriority
    local cl_delrecord
    local cl_force
    local cl_unicode
    local cl_decimalto
    local cl_delchar
    local cl_delfield
    local cl_datefmt
    local cl_timestampfmt
    local cl_trim
    local cl_headerline
    local cl_sparse
    local cl_extra
    local cl_cast
    local cl_strictfieldnum
    local cl_checkdelimeter
    local cl_params
    while [ "$1" != "" ]; do
        opt=${1%%=*}
        value=${1#*=}
        if [ "$value" = "" ]; then
            continue
        fi
        value="${value//$REPLACE_STR/ }"
        value=$(dealQuotes $opt "$value")
        case $opt in
            type           ) repeatCheck "$opt" "$cl_type";           cl_type=$value           ;;
            linepriority   ) repeatCheck "$opt" "$cl_linepriority";   cl_linepriority=$value   ;;
            delrecord      ) repeatCheck "$opt" "$cl_delrecord";      cl_delrecord=$value      ;;
            force          ) repeatCheck "$opt" "$cl_force";          cl_force=$value          ;;
            unicode        ) repeatCheck "$opt" "$cl_unicode";        cl_unicode=$value        ;;
            decimalto      ) repeatCheck "$opt" "$cl_decimalto";      cl_decimalto=$value      ;;
            delchar        ) repeatCheck "$opt" "$cl_delchar";        cl_delchar=$value        ;;
            delfield       ) repeatCheck "$opt" "$cl_delfield";       cl_delfield=$value       ;;
            datefmt        ) repeatCheck "$opt" "$cl_datefmt";        cl_datefmt=$value        ;;
            timestampfmt   ) repeatCheck "$opt" "$cl_timestampfmt";   cl_timestampfmt=$value   ;;
            trim           ) repeatCheck "$opt" "$cl_trim";           cl_trim=$value           ;;
            headerline     ) repeatCheck "$opt" "$cl_headerline";     cl_headerline=$value     ;;
            sparse         ) repeatCheck "$opt" "$cl_sparse";         cl_sparse=$value         ;;
            extra          ) repeatCheck "$opt" "$cl_extra";          cl_extra=$value          ;;
            cast           ) repeatCheck "$opt" "$cl_cast";           cl_cast=$value           ;;
            strictfieldnum ) repeatCheck "$opt" "$cl_strictfieldnum"; cl_strictfieldnum=$value ;;
            checkdelimeter ) repeatCheck "$opt" "$cl_checkdelimeter"; cl_checkdelimeter=$value ;;
        esac
        shift
    done
    cl_type=$(appendParam           '--type'            "$cl_type"           "$opt_type")
    cl_linepriority=$(appendParam   '--linepriority'    "$cl_linepriority"   "$opt_linepriority")
    cl_delrecord=$(appendParam      '--delrecord'       "$cl_delrecord"      "$opt_delrecord")
    cl_force=$(appendParam          '--force'           "$cl_force"          "$opt_force")
    cl_unicode=$(appendParam        '--unicode'         "$cl_unicode"        "$opt_unicode")
    cl_decimalto=$(appendParam      '--decimalto'       "$cl_decimalto"      "$opt_decimalto")
    cl_delchar=$(appendParam        '--delchar'         "$cl_delchar"        "$opt_delchar")
    cl_delfield=$(appendParam       '--delfield'        "$cl_delfield"       "$opt_delfield")
    cl_datefmt=$(appendParam        '--datefmt'         "$cl_datefmt"        "$opt_datefmt")
    cl_timestampfmt=$(appendParam   '--timestampfmt'    "$cl_timestampfmt"   "$opt_timestampfmt")
    cl_trim=$(appendParam           '--trim'            "$cl_trim"           "$opt_trim")
    cl_headerline=$(appendParam     '--headerline'      "$cl_headerline"     "$opt_headerline")
    cl_sparse=$(appendParam         '--sparse'          "$cl_sparse"         "$opt_sparse")
    cl_extra=$(appendParam          '--extra'           "$cl_extra"          "$opt_extra")
    cl_cast=$(appendParam           '--cast'            "$cl_cast"           "$opt_cast")
    cl_strictfieldnum=$(appendParam '--strictfieldnum'  "$cl_strictfieldnum" "$opt_strictfieldnum")
    cl_checkdelimeter=$(appendParam '--checkdelimeter'  "$cl_checkdelimeter" "$opt_checkdelimeter")

    cl_params=$(arrToStr "$cl_type" "$cl_linepriority" "$cl_delrecord" "$cl_force" "$cl_unicode" "$cl_decimalto" "$cl_delchar" "$cl_delfield" "$cl_datefmt" "$cl_timestampfmt" "$cl_trim" "$cl_headerline" "$cl_sparse" "$cl_extra" "$cl_cast" "$cl_strictfieldnum" "$cl_checkdelimeter")
    echo "$cl_params"
}

function buildCLParams()
{
    local cl_param
    local opt
    local value
    local cl_name
    local cl_file
    local cl_fields
    local cl_exec
    local cl_full_name
    local other_params=""
    local cl_general_params=""
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
            name   ) repeatCheck "$opt" "$cl_name";   cl_name="$(genCSCLParam $value)";
                     cl_full_name=$value                                             ;;
            file   ) repeatCheck "$opt" "$cl_file";   cl_file="--file $value"        ;;
            exec   ) repeatCheck "$opt" "$cl_exec";   cl_exec="--exec $value"        ;;
            fields ) repeatCheck "$opt" "$cl_fields"; cl_fields="--fields $value"    ;;
            *      ) other_params="$other_params $1"                                 ;;
        esac
        shift
    done
    if [ "$cl_full_name" = "" ]; then
        echo "[ERROR] Collection name must be specified"
        exit 1
    fi
    if [ "$cl_file" = "" -a "$cl_exec" = "" ]; then
        echo "[ERROR] Collection: $cl_full_name : The import data must be specified!"
        exit 1
    fi

    cl_general_params=$(parseGeneralParams $other_params)
    if [ $? != 0 ]; then
        echo "[ERROR] ${cl_general_params[@]}"
        exit 1
    fi

    cl_param=$(arrToStr $cl_name $cl_file "$cl_fields" $cl_exec "$cl_general_params")
    if [ "$cl_param" != ""  ]; then
        cl_params_list[${#cl_params_list[@]}]="$cl_param"
    fi
}

function buildCSParams()
{
    local opt
    local value
    local cs_name
    local excludecl
    local dir
    local arr
    local cl_param
    local cl_list
    local cl_full_name
    local cl_file_param
    local cl_fields_param
    local cl_general_params
    local other_params
    local fields_list=()
    while [ "$1" != "" ]; do
        opt=${1%%=*}
        value=${1#*=}
        if [ "$value" = "" ]; then
            shift
            continue
        fi
        value=$(dealQuotes $opt "$value")
        case $opt in
            name      ) repeatCheck "$opt" "$cs_name";   cs_name="${value//$REPLACE_STR/ }"   ;;
            excludecl ) repeatCheck "$opt" "$excludecl"; excludecl="${value//$REPLACE_STR/ }" ;;
            dir       ) repeatCheck "$opt" "$dir";       dir="${value//$REPLACE_STR/ }"       ;;
            fields    ) fields_list[${#fields_list[@]}]="$value"                              ;;
            *         ) other_params="$other_params $1"                                       ;;
        esac
        shift
    done

    if [ "$cs_name" = "" -o "$dir" = "" ]; then
        echo "[ERROR] CollectionSpace name or import data files must be specified"
        exit 1
    fi

    cl_general_params=$(parseGeneralParams $other_params)
    if [ $? != 0 ]; then
        echo "[ERROR] ${cl_general_params[@]}"
        exit 1
    fi

    cl_list=$(getCLFromDir "$dir" "$cs_name" "$excludecl,")
    if [ "${cl_list[@]}" = "" ]; then
        addResultBody "There are no available files under directory '$dir'\n"
        return 0
    fi
    for cl_name in ${cl_list[@]}; do
        cl_file_param=""
        cl_fields_param="$(paresFields $cs_name.$cl_name ${fields_list[*]})"
        cl_file=$(getFileParam "$cs_name.$cl_name.*" "$dir")

        cs_name_param=$(appendParam '--csname' $cs_name)
        cl_name=$(appendParam       '--clname' $cl_name)
        cl_file=$(appendParam       '--file'   $cl_file)
        if [ "$cl_fields_param" != "" ]; then
            cl_fields_param="--fields $cl_fields_param"
        fi

        cl_param=$(arrToStr $cs_name_param $cl_name $cl_file $cl_fields_param $cl_general_params)
        if [ "$cl_param" != "" ]; then
            cl_params_list[${#cl_params_list[@]}]="$cl_param"
        fi
    done
}

function getFileParam()
{
    local file_str=$1
    local file_list=(${2//,/ })
    local file
    local result=""
    for dir in ${file_list[@]}; do
        for file in `find "$dir" -name "$file_str"`; do
            result="$result$file,"
        done
    done
    if [ "${#result}" != 0 ]; then
        echo "${result:0:${#result}-1}"
    else
        echo ""
    fi
}

function getCLFromDir()
{
    local cs_name=$2
    local excludecl=(${3//,/ })
    local dir=$1
    local cl_list=()
    local cl_name
    local cs_name_tmp
    local array
    local file
    if [ "$cs_name" = "" ]; then
        return 1
    fi
    if test -d $dir; then
        for file in `ls $dir`; do
            array=(${file//./ })
            if [ ${#array[@]} -le 1 ]; then
                continue
            fi
            cs_name_tmp=${array[0]}
            if [ "$cs_name" != "$cs_name_tmp" ]; then
                continue
            fi
            cl_name=${array[1]}
            contains $cl_name "${excludecl[*]}"
            if [ $? -eq 0 ]; then
               continue
            fi
            contains $cl_name "${cl_list[*]}"
            if [ $? -eq 0 ]; then
               continue
            fi
            cl_list[${#cl_list[@]}]=$cl_name
        done
    fi
    echo "${cl_list[@]}"
}

function parseImportConf()
{
    local i
    local cl_sct
    local cl_param
    local cs_param
    local param
    local msg

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

function parseCmdParam()
{
    if [ "$opt_csname" = "" -o "$opt_clname" = "" ]; then
        return 0
    fi
    buildCLParams "name=$opt_csname.$opt_clname" "file=$opt_file" "fields=$opt_fields" "exec=$opt_exec"
}

function buildImportCommand()
{
    #get cl params from cmd
    parseCmdParam

    #get cl params from conf file
    parseImportConf

    if [ "$opt_verbose" = "true" ]; then
        opt_general_params="$opt_general_params --verbose"
    fi

    for ((i=0; i<${#cl_params_list[@]}; i++)); do
        import_cmd_list[i]="$import_tool ${cl_params_list[$i]} $opt_general_params"
    done
}

function findInfo()
{
    local item="$1"
    local msg="$2"
    local arr
    if [ $# -lt 1 ]; then
        echo ""
        return 1
    fi
    line=$(echo "$msg" | sed -n "/$item/p")
    if [ "$line" = "" ]; then
        return 1
    fi
    arr=(${line##*:})
    echo "$arr"
    return 0
}

function judgeResult()
{
    local msg="$1"
    findInfo "failed to parse fields" "$msg"
    if [ $? = 0 ]; then
        return 1
    fi
    local parse_faile=$(findInfo "parse failure" "$msg")
    local sharding_faile=$(findInfo "sharding failure" "$msg")
    local import_faile=$(findInfo "import failure" "$msg")

   if [ "$parse_faile" = "0" -a "$sharding_faile" = "0" -a "$import_faile" = "0" ]; then
       return 0
   else
       return 1
   fi
}

function genResultHead()
{
    faile_num=$[total_num-success_num]
    local consume_time=$(countTimeD "$start_time" "$end_time")
    local str_1="========== Import result =========="
    local str_2="Imported count   : $total_num"
    local str_3="Successful count : $success_num"
    local str_4="Failed count     : $faile_num"
    local str_5="Start time       : $start_time"
    local str_6="End time         : $end_time"
    local str_7="Consume time     : $consume_time"

    result_head="$str_1\n$str_2\n$str_3\n$str_4\n$str_5\n$str_6\n$str_7\n"
}

function addResultBody()
{
    if [ "$result_body" = "" ]; then
        result_body="---------- Import detail ----------"
    fi
    if [ $# -ne 0 ]; then
        result_body="$result_body\n[Import]\n$@"
        total_num=$[total_num+1]
    fi
}

function importData()
{
    local i
    local import_cmd
    local cl
    local rc=0
    local msg
    # touch import.result
    genResultFile "$import_result_file"

    buildImportCommand

    for ((i=0; i<${#import_cmd_list[@]}; i++)); do
        import_cmd=${import_cmd_list[i]}
        cl=$(getCLInfo $import_cmd)
        msg=$(eval "$import_cmd" 2>&1)
        rc=$?
        if [ $rc = 0 ]; then
            judgeResult "$msg"
            if [ $? = 0 ]; then
                success_num=$[success_num+1]
            fi
        fi

        if [ $rc != 0 -o "$opt_debug" = "true" ]; then
            msg="Collection:$cl\n$msg\nImport cmd: $import_cmd\n"
        else
            msg="Collection:$cl\n$msg\n"
        fi
        addResultBody "$msg"
    done

    end_time=$(date +'%Y-%m-%d %H:%M:%S')

    genResultHead

    echo -e "$result_head\n$result_body" >> $import_result_file

    echo "Import finish, view the details from file $import_result_file"
}

function main()
{
    start_time=$(date +'%Y-%m-%d %H:%M:%S')
    import_tool=$(findTool $import_tool)
    if [ $? -ne 0 ]; then
        echo "$import_tool"
        return 1
    fi

    parseParam $@

    importData
}

main $@
