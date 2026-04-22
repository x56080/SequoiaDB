#!/bin/bash

usage() {
    echo "Usage: $0 -d <directory> [options]"
    echo "Options:"
    echo "  -d, --directory <dir>   : 指定节点 diaglog 日志目录 (必需指定)"
    echo "  -l, --level <level>     : 按日志级别过滤 (0-4: 0=SEVERE, 1=ERROR, 2=EVENT, 3=WARNING, 4=INFO, 包含更低级别)"
    echo "  -p, --pid <pid>         : 按进程ID过滤"
    echo "  -t, --tid <tid>         : 按线程ID过滤"
    echo "  -E, --error <errorcode> : 在 Message 中搜索错误码"
    echo "  -m, --message <keyword> : 在 Message 中搜索关键字"
    echo "  -s, --start <datetime>  : 过滤此时间之后的日志 (格式: YYYY-MM-DD-HH.MM.SS)"
    echo "  -e, --end <datetime>    : 过滤此时间之前的日志 (格式: YYYY-MM-DD-HH.MM.SS)"
    echo "  -r, --recent <minutes>  : 搜索最近 <minutes> 分钟的日志 (与 -s/--start, -e/--end 参数互斥)"
    echo "  -b, --before <num>      : 包含匹配日志的前 <num> 条日志"
    echo "  -a, --after <num>       : 包含匹配日志的后 <num> 条日志"
    echo "  -n, --limit <num>       : 限制输出的日志条数"
    echo "  -f, --files <num>       : 限制搜索最近的 <num> 个日志文件"
    echo "  -o, --output <file>     : 输出结果到指定文件，默认打屏"
    echo "  -F, --files-only        : 仅输出匹配日志的文件名"
    echo "  -O, --original          : 输出原始日志，默认为一行的简要格式"
    echo "  -H, --host              : 指定输出中 HostName 的值，不指定时为空"
    echo "  -S, --service           : 指定输出中 ServiceName 的值，不指定时为空"
    echo "  -R, --role              : 指定输出中 Role 的值，不指定时为空"
    echo "  -h, --help              : 显示帮助信息"
    echo "Example:"
    echo "  查找日志中出现的 -34 错误，限制 10 条"
    echo "  ./logSearchTool.sh -d /opt/sequoiadb/database/coord/diaglog -E '-34' -n 10"
    echo "  查找日志中出现的 -34 错误，限制 10 条, 仅查找最近 30 分钟的日志"
    echo "  ./logSearchTool.sh -d /opt/sequoiadb/database/coord/diaglog -E '-34' -n 10 -r 30"
    echo "  查找日志中出现的 -34 错误，限制 10 条, 仅查找最近的一个日志文件"
    echo "  ./logSearchTool.sh -d /opt/sequoiadb/database/coord/diaglog -E '-34' -n 10 -f 1"
    echo "  查找日志中 Message: 出现的 failed 关键字，限制 10 条, pid 为 123456, tid 为 56789, 日志级别为 ERROR 及以下"
    echo "  ./logSearchTool.sh -d /opt/sequoiadb/database/coord/diaglog -m 'failed' -n 10 -p 12345 -t 56789 -l 1"
    echo "Optimize:"
    echo "  指定以下搜索条件时，工具可以优化搜索方法，可以加快搜索速度:"
    echo "    -l, --level <level>"
    echo "    -p, --pid <pid>"
    echo "    -t, --tid <tid>" 
    echo "    -E, --error <errorcode>"
    echo "    -m, --message <keyword>"
    echo "    -n, --limit <num>"
    echo "    -f, --files <num>"
    echo "    -O, --original"
    echo "  指定以下搜索条件时，会使工具无法使用除 limit 以外的任何优化，搜索速度会很慢:"
    echo "    -a, --after <num>"
    echo "    -b, --before <num>"
}

normalize_time() {
    if ! [[ $1 =~ ^[0-9.:-]+$ ]]; then
        echo "[ERROR] Failed to parse time: $1" >&2
        return 1
    fi

    local year="${1%%-*}"
    if [[ $year =~ ^[0-9]{4}$ ]]; then
        :
    else
        echo "[ERROR] Failed to parse time: $1" >&2
        return 1
    fi
    local normalized=$(echo "$1" | sed 's/[-:.]//g')
    echo "${normalized}00000000000000" | cut -c1-14
    return 0
}

# 变为标准格式如 2026-01-01-12.01.01
standard_time() {
    local str="${1}"
    if ! [[ $str =~ ^[0-9]{14}$ ]]; then
        echo "[ERROR] Failed to parse time: $1" >&2
        return 1
    fi
    local year="${str:0:4}"
    local month="${str:4:2}"
    local day="${str:6:2}"
    local hour="${str:8:2}"
    local minutes="${str:10:2}"
    local seconds="${str:12:2}"
    echo "${year}-${month}-${day}-${hour}.${minutes}.${seconds}"
    return 0
}

filename_to_time() {
    local filename=$1
    if [[ "$filename" =~ \.([0-9]{4}-[0-9]{2}-[0-9]{2}-[0-9]{2}:[0-9]{2}:[0-9]{2})$ ]]; then
        normalize_time "${filename##*.}"
        return $?
    else
        echo ""
    fi
    return 0
}

# 指定按 ASCII 处理文本
LC_ALL=C

# 参数初始化
LEVEL=""
LEVEL_NUM=""
PID=""
TID=""
ERROR=""
MSG_KEYWORD=""
START_TIMESTR=""
END_TIMESTR=""
START_TIME=""
END_TIME=""
RECENT_MINUTES=""
CONTEXT_BEFORE=""
CONTEXT_AFTER=""
LIMIT=""
FILE_LIMIT=""
OUTPUT_FILE=""
ORIGINAL_OUTPUT=""
FILES_ONLY=false
SERVICE_NAME=""
HOST_NAME=""
NODE_TYPE=""
SORT_ONLY=false
LOG_DIR=""
NEEDSEARCH=false
CONDITION_COUNT=0

TEMP=$(getopt -o d:l:p:t:E:m:s:e:r:b:a:n:f:o:H:S:R:OFh --long directory:,level:,pid:,tid:,error:,message:,start:,end:,recent:,before:,after:,limit:,files:,output:,host:,service:,role:,original,files-only,sort-only,help -n "$0" -- "$@")

if [ $? != 0 ]; then
    usage
    exit 1
fi

eval set -- "$TEMP"

while true; do
    case "$1" in
        -d|--directory)
            LOG_DIR="$2"
            shift 2
            ;;
        -l|--level)
            LEVEL_NUM="$2"
            ((CONDITION_COUNT++))
            shift 2
            ;;
        -p|--pid)
            PID="$2"
            ((CONDITION_COUNT++))
            shift 2
            ;;
        -t|--tid)
            TID="$2"
            ((CONDITION_COUNT++))
            shift 2
            ;;
        -E|--error)
            ERROR="$2"
            ((CONDITION_COUNT++))
            shift 2
            ;;
        -m|--message)
            MSG_KEYWORD="$2"
            ((CONDITION_COUNT++))
            shift 2
            ;;
        -s|--start)
            ((CONDITION_COUNT++))
            START_TIME=$(normalize_time "$2")
            test $? -ne 0 && exit 1
            START_TIMESTR=$(standard_time "$START_TIME")
            test $? -ne 0 && exit 1
            shift 2
            ;;
        -e|--end)
            ((CONDITION_COUNT++))
            END_TIME=$(normalize_time "$2")
            test $? -ne 0 && exit 1
            END_TIMESTR=$(standard_time "$END_TIME")
            test $? -ne 0 && exit 1
            shift 2
            ;;
        -r|--recent)
            RECENT_MINUTES="$2"
            ((CONDITION_COUNT++))
            shift 2
            ;;
        -b|--before)
            CONTEXT_BEFORE="$2"
            ((CONDITION_COUNT++))
            shift 2
            ;;
        -a|--after)
            CONTEXT_AFTER="$2"
            ((CONDITION_COUNT++))
            shift 2
            ;;
        -n|--limit)
            LIMIT="$2"
            shift 2
            ;;
        -f|--files)
            FILE_LIMIT="$2"
            shift 2
            ;;
        -o|--output)
            OUTPUT_FILE="$2"
            shift 2
            ;;
        -O|--original)
            ORIGINAL_OUTPUT=true
            shift
            ;;
        -F|--files-only)
            FILES_ONLY=true
            shift
            ;;
        -H|--host)
            HOST_NAME="$2"
            shift 2
            ;;
        -S|--service)
            SERVICE_NAME="$2"
            shift 2
            ;;
        -R|--role)
            NODE_ROLE="$2"
            shift 2
            ;;
        --sort-only)
            SORT_ONLY=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "[ERROR] Unknown parameters: $1" >&2
            usage
            exit 1
            ;;
    esac
done

if [ -n "$LIMIT" ]; then
    if ! [[ "$LIMIT" =~ ^[1-9][0-9]*$ ]]; then
        echo '[ERROR] Parameter "-n/--limit" must be greate than 0' >&2
        exit 1
    fi
fi

if [ -n "$OUTPUT_FILE" ]; then
    if [[ "$OUTPUT_FILE" == /* ]]; then
        # 绝对路径
        :
    else
        # 相对路径
        OUTPUT_FILE=`pwd`"/$OUTPUT_FILE"
    fi
    dir=$(dirname "$OUTPUT_FILE")
    mkdir -p "$dir"
    test $? -ne 0 && echo "[ERROR] Failed to mkdir \"$dir\"" >&2
    chmod 777 "$dir"
    test $? -ne 0 && echo "[ERROR] Failed to chmod 777 \"$dir\"" >&2
    if [ ! -r "$dir" ]; then
        echo "[ERROR] Cannot read \"$dir\"" >&2
        exit 1
    fi
    if [ ! -w "$dir" ]; then
        echo "[ERROR] Cannot write \"$dir\"" >&2
        exit 1
    fi
fi

# 内部排序函数，外部不可见。带 主机名:端口号 进行排序(一段日志8行)
if [ "$SORT_ONLY" == true ]; then
    if [ "$OUTPUT_FILE" == "" ]; then
        echo '[ERROR] Parameter "--sort-only" and "-o/--output" must be used together' >&2
        exit 1
    fi

    line=`wc -l < "$OUTPUT_FILE"`
    test "$line" == "0" && echo "[WARNING] \"$OUTPUT_FILE\" is Empty" && exit 0

    temp_results=$(mktemp)
    test $? -ne 0 && echo '[ERROR] Failed to exec "mktemp"' >&2 && exit 1

    # 仅排序
    if [ "$ORIGINAL_OUTPUT" == true ]; then
        awk -v RS='' -v ORS='\n\n' '{gsub(/\n/,"\\n"); print}' "$OUTPUT_FILE" | grep -av '^$' > "$temp_results"
        # 确认是否带有 =====nodename===== 行，需要使用不同的排序方法
        first_line=`head -n 1 "$temp_results" | grep -a '^======'`
        if [ "" != "$first_line" ]; then
            if [ -z "$LIMIT" ]; then
                sort -rt'\' -k2,2 "$temp_results" | sed 's/\\n/\n/g' | awk '{if(NR==1){print;next}else if($0 ~ /^======/){print "\n" $0}else{print}}' > "$OUTPUT_FILE"
            else
                sort -rt'\' -k2,2 "$temp_results" | head -n $LIMIT | sed 's/\\n/\n/g' | awk '{if(NR==1){print;next}else if($0 ~ /^======/){print "\n" $0}else{print}}' > "$OUTPUT_FILE"
            fi
        else
            if [ -z "$LIMIT" ]; then
                sort -rt'\' -k2,2 "$temp_results" | sed 's/\\n/\n/g' | awk '{if(NR==1){print;next}else if($0 ~ /^[0-9]{4}-[0-9]{2}/){print "\n" $0}else{print}}' > "$OUTPUT_FILE"
            else
                sort -rt'\' -k2,2 "$temp_results" | head -n $LIMIT | sed 's/\\n/\n/g' | awk '{if(NR==1){print;next}else if($0 ~ /^[0-9]{4}-[0-9]{2}/){print "\n" $0}else{print}}' > "$OUTPUT_FILE"
            fi
        fi
    else
        cp "$OUTPUT_FILE" "$temp_results"
        if [ -z "$LIMIT" ]; then
            sort -rt, -k4,4 "$temp_results" > "$OUTPUT_FILE"
        else
            sort -rt, -k4,4 "$temp_results" | head -n $LIMIT > "$OUTPUT_FILE"
        fi
    fi
    rm -f "$temp_results"
    exit 0
fi

if [ -z "$LOG_DIR" ]; then
    echo '[ERROR] Parameter "-d/--directory" must be specified' >&2
    exit 1
fi
LOG_DIR="${LOG_DIR%/}"

if [ ! -d "$LOG_DIR" ]; then
    echo "[ERROR] \"$LOG_DIR\" does not exist" >&2
    exit 1
fi

if [ ! -r "$LOG_DIR" ]; then
    echo "[ERROR] Cannot read \"$LOG_DIR\"" >&2
    exit 1
fi

if [ -n "$RECENT_MINUTES" ] && ([ -n "$START_TIME" ] || [ -n "$END_TIME" ]); then
    echo '[ERROR] Parameter "-r/--recent" cannot be used with "-s/--start" or "-e/--end"' >&2
    exit 1
fi

if [ -n "$START_TIME" ] && [ -n "$END_TIME" ]; then
    if [[ $START_TIME -ge $END_TIME ]]; then
        echo '[ERROR] start time must be letter than end time' >&2
        exit 1
    fi
fi

if [ -n "$TID" ]; then
    if ! [[ "$TID" =~ ^[1-9][0-9]*$ ]]; then
        echo '[ERROR] Parameter "-t/--tid" must be greate than 0' >&2
        exit 1
    fi
    NEEDSEARCH=true
fi

if [ -n "$PID" ]; then
    if ! [[ "$PID" =~ ^[1-9][0-9]*$ ]]; then
        echo '[ERROR] Parameter "-p/--pid" must be greate than 0' >&2
        exit 1
    fi
    NEEDSEARCH=true
fi

if [ -n "$CONTEXT_AFTER" ]; then
    if ! [[ "$CONTEXT_AFTER" =~ ^[0-9]*$ ]]; then
        echo '[ERROR] Parameter "-a/--after" must be greate than 0' >&2
        exit 1
    fi
    if [ "$CONTEXT_AFTER" == "0" ]; then
        CONTEXT_AFTER=""
    fi
fi

if [ -n "$CONTEXT_BEFORE" ]; then
    if ! [[ "$CONTEXT_BEFORE" =~ ^[0-9]*$ ]]; then
        echo '[ERROR] Parameter "-b/--before" must be greate than 0' >&2
        exit 1
    fi
    if [ "$CONTEXT_BEFORE" == "0" ]; then
        CONTEXT_BEFORE=""
    fi
fi

if [ -n "$LEVEL_NUM" ]; then
    if ! [[ "$LEVEL_NUM" =~ ^[0-4]$ ]]; then
        echo '[ERROR] Parameter "-l/--level" must be [0-4]' >&2
        exit 1
    fi
fi

if [ -n "$FILE_LIMIT" ]; then
    if ! [[ "$FILE_LIMIT" =~ ^[1-9][0-9]*$ ]]; then
        echo '[ERROR] Parameter "-f/--files" must be greate than 0' >&2
        exit 1
    fi
fi

if [ -n "$ERROR" ]; then
    if ! [[ "$ERROR" =~ ^-[1-9][0-9]*$ ]]; then
        echo '[ERROR] Parameter "-E/--error" must be less than 0' >&2
        exit 1
    fi
    NEEDSEARCH=true
fi

if [ -n "$MSG_KEYWORD" ]; then
    NEEDSEARCH=true
fi

if [ -n "$RECENT_MINUTES" ]; then
    if ! [[ "$RECENT_MINUTES" =~ ^[1-9][0-9]*$ ]]; then
        echo '[ERROR] Parameter "-r/--recent" must be greate than 0' >&2
        exit 1
    fi
    
    current_epoch=$(date '+%s')
    start_epoch=$((current_epoch - RECENT_MINUTES * 60))
    
    START_TIME=$(date -d "@$start_epoch" '+%Y%m%d%H%M%S000000' 2>/dev/null)
    START_TIMESTR=$(date -d "@$start_epoch" '+%Y-%m-%d-%H.%M.%S.000000' 2>/dev/null)

    if [ $? -ne 0 ] || [ -z "$START_TIME" ]; then
        echo '[ERROR] Failed to calculate the recent time from "-r|--recent"' >&2
        exit 1
    fi
fi

if [ "$FILES_ONLY" == true ]; then
    # 只需要文件名不需要上下文
    CONTEXT_BEFORE=""
    CONTEXT_AFTER=""
    # 简要模式需要保存行号，原始模式不需要，因此原始模式速度更快
    ORIGINAL_OUTPUT=true
fi

if [[ (-z "$HOST_NAME" && -z "$SERVICE_NAME" && -z "$NODE_ROLE") || (! -z "$HOST_NAME" && ! -z "$SERVICE_NAME" && ! -z "$NODE_ROLE") ]]; then
    :
else
    echo '[ERROR] Parameter "-H/--host", "-R/--role" and "-S/--service" must be used together' >&2
    exit 1
fi

if [ "$NEEDSEARCH" == false ]; then
    echo '[INFO] No searchable conditions, such as "-E/--error", "-m/--message", "-t/--tid" or "-p/--pid"' >&2
    exit 0
fi

get_log_files() {
    local files=()
    if [ -f "$LOG_DIR/sdbdiag.log" ]; then
        files+=("$LOG_DIR/sdbdiag.log")
        if [ ! -r "$LOG_DIR/sdbdiag.log" ]; then
            echo "[ERROR] Cannot read \"$LOG_DIR/sdbdiag.log\"" >&2
            files=()
        fi
    fi
    for file in $(ls "$LOG_DIR/sdbdiag.log."* 2>/dev/null | sort -r); do
        files+=("$file")
        if [ ! -r "$file" ]; then
            echo "[ERROR] Cannot read \"$file\"" >&2
            files=()
        fi
    done
    if [ "${#files[@]}" -gt "0" ]; then
        printf '%s\n' "${files[@]}"
    fi
}

temp_results=$(mktemp)
test $? -ne 0 && echo '[ERROR] Failed to exec "mktemp"' >&2 && exit 1

readarray -t log_files < <(get_log_files)

if [ "${#log_files[@]}" -le "0" ]; then
    echo "[INFO] No diaglog.log files were found in the directory \"$LOG_DIR\"" >&2
    # 创建空文件以标识完成搜索
    touch "$OUTPUT_FILE"
    exit $?
fi

if [ -n "$FILE_LIMIT" ] && [ "${#log_files[@]}" -gt "$FILE_LIMIT" ]; then
    log_files=("${log_files[@]:0:$FILE_LIMIT}")
fi

total_matched=0
matched_files=()
total_log_count=0

for file in "${log_files[@]}"; do
    file_time=$(filename_to_time "$file")
    real_file=$file
    
    # 文件名时间范围快速检查 (使用字符串比较)
    # 因为文件末尾的时间是最近一条日志的时间，所以只能比较 start time, end time 无法比较
    [ -n "$START_TIME" ] && [ -n "$file_time" ] && [[ "$file_time" < "$START_TIME" ]] && continue
    
    if [ "$FILES_ONLY" = false ]; then
        echo "[INFO] Searching for file: $file" >&2
    fi

    grepCmd=""
    condCount=$CONDITION_COUNT
    if [ "$ORIGINAL_OUTPUT" != true ] ;then
        # 简要模式需要带上 Message 的行号
        grepCmd="awk '
            # 拼接 message 行
            {
                if (in_message) {
                    # 丢弃 Message: 后面多行内容，只打印一行
                    print \"Message:\" msg_line_num \"|\" \$0 \"\n\"
                    in_message = 0
                    next;
                } else if (\$0 == \"Message:\") {
                    # 标记 message 行
                    in_message = 1
                    msg_line_num = NR
                    next;
                }

                # 直接输出非 Message 的行
                print
            }
        ' $file"
    fi

    # 提前使用 grep 优化性能
    # 标记是否可以继续优化
    canOptimized=true
    onlyLevel=true

    # 使用 -a 和 -b 无法优化
    if [[ "$CONTEXT_BEFORE" != "" || "$CONTEXT_AFTER" != "" ]]; then
        canOptimized=false
        grepCmd=""
    fi

    # 过滤 TID
    if [[ "$canOptimized" == true && "$TID" != "" ]]; then
        ((condCount--))
        if [ "$grepCmd" == "" ]; then
            grepCmd="grep -a -B 1 -A 5 \"TID:$TID\\\$\" $file"
        else
            grepCmd="${grepCmd} | grep -a -B 1 -A 5 \"TID:$TID\\\$\" $file"
        fi
        onlyLevel=false
    fi

    # 过滤 PID
    if [[ "$canOptimized" == true && "$PID" != "" ]]; then
        ((condCount--))
        if [ "$grepCmd" == "" ]; then
            grepCmd="grep -a -B 1 -A 5 \"^PID:$PID\" $file"
        else
            grepCmd="${grepCmd} | grep -a -B 1 -A 5 \"^PID:$PID\" $file"
        fi
        onlyLevel=false
    fi

    if [[ "$MSG_KEYWORD" != "" || "$ERROR" != "" ]]; then
        onlyLevel=false
    fi

    # 过滤 LEVEL
    if [[ "$canOptimized" == true && "$LEVEL_NUM" != "" ]]; then
        # 如果只有 Level 条件，并且过滤等级大于 2（WARNING, INFO），性能多数情况下会更差，则不进行提前过滤
        if [[ $LEVEL_NUM -le 2 || "$onlyLevel" == false ]]; then
            ((condCount--))
            level_names=("ERROR" "EVENT" "WARNING" "INFO")
            level_pattern="Level:SEVERE\\\$"
            for ((i = 0; i <= LEVEL_NUM - 1; i++))
            do
                level_pattern="${level_pattern}|Level:${level_names[i]}\\\$"
            done

            if [ "$grepCmd" == "" ]; then
                grepCmd="grep -a -A $((grepAfter + 6)) -E \"$level_pattern\" $file"
            else
                grepCmd="${grepCmd} | grep -a -A $((grepAfter + 6)) -E \"$level_pattern\""
            fi
        fi
    fi

    # 过滤 key 和 error
    optimizeMsg=false
    if [[ "$canOptimized" == true && ( "$MSG_KEYWORD" != "" || "$ERROR" != "" ) ]]; then
        ((condCount--))
        if [[ "$MSG_KEYWORD" != "" && "$ERROR" != "" ]]; then
            ((condCount--))
        fi
        optimizeMsg=true

        # 如果只需要文件名，满足所有条件，而且没有 limit，则不需要对 Message 行进行额外处理
        if [[ "$FILES_ONLY" == true && -z "$LIMIT" && "0" == "$condCount" ]]; then
            if [ "$grepCmd" == "" ]; then
                grepCmd="grep -a -A 1 '^Message:$' $file | grep -av '^Message:$' | grep -av '^--$'"
            else
                grepCmd="${grepCmd} | grep -a -A 1 '^Message:$' | grep -av '^Message:$' | grep -av '^--$'"
            fi

            if [[ "$MSG_KEYWORD" != "" && "$ERROR" != "" ]]; then
                grepCmd="${grepCmd} | grep -aE -- '$ERROR]?$' | grep -aq '$MSG_KEYWORD'"
            elif [ "$MSG_KEYWORD" != "" ]; then
                grepCmd="${grepCmd} | grep -aq '$MSG_KEYWORD'"
            elif [ "$ERROR" != "" ]; then
                grepCmd="${grepCmd} | grep -aqE -- '$ERROR]?$'"
            fi
        else
            if [ "$ORIGINAL_OUTPUT" == true ]; then
                if [ "$grepCmd" == "" ]; then
                    grepCmd="sed '/^Message:$/{N;s/\n//}' $file"
                else
                    grepCmd="${grepCmd} | sed '/^Message:$/{N;s/\n//}'"
                fi
            fi

            if [[ "$MSG_KEYWORD" != "" && "$ERROR" != "" ]]; then
                grepCmd="${grepCmd} | grep -a -B 4 -A 1 -- '^Message:.*$ERROR]?$' | grep -a -B 4 -A 1 '^Message:.*$MSG_KEYWORD.*'"
            elif [ "$MSG_KEYWORD" != "" ]; then
                grepCmd="${grepCmd} | grep -a -B 4 -A 1 '^Message:.*$MSG_KEYWORD.*'"
            elif [ "$ERROR" != "" ]; then
                grepCmd="${grepCmd} | grep -a -B 4 -A 1 -E -- '^Message:.*$ERROR]?$'"
            fi
        fi
    fi

    # 限制条数，只有条件完整时才能做 limit
    isLimit=""
    if [[ -n "$LIMIT" && "0" == "$condCount" ]]; then
        grepCmd="${grepCmd} | grep -av '^--\$' | tail -n $((LIMIT * 6))"
        # 如果能做 limit，代表所有条件都满足，已经找出了结果，后续只需要简单处理格式，可以把所有条件都忽略，跳过后续二次判断
        isLimit=true
    fi

    # 如果只需要文件名，满足所有条件，且没有指定 limit，使用 grep 快速确认本文件是否有目标内容，不需要考虑具体数量
    if [[ "$FILES_ONLY" == true && -z "$LIMIT" && "0" == "$condCount" ]]; then
        eval "${grepCmd}" > /dev/null
        if [ $? -eq 0 ]; then
            # 匹配成功，保存文件名
            if [[ ! -z "${HOST_NAME}" && ! -z "${SERVICE_NAME}" && ! -z "${NODE_ROLE}" ]]; then
                matched_files+=("${HOST_NAME}@@${SERVICE_NAME}@@${real_file}")
            else
                matched_files+=("${real_file}")
            fi
            total_matched=$((total_matched + 1))
        fi
        # 本文件已搜索完成，进行下一个文件
        continue
    fi

    if [ "$optimizeMsg" == true ]; then
        if [ "$isLimit" != true ]; then
            grepCmd="${grepCmd} | grep -av '^--\$'"
        fi

        if [ "$ORIGINAL_OUTPUT" == true ]; then
            grepCmd="${grepCmd} | sed 's/^Message:/Message:\n/' | sed 's/^\([0-9]\{4\}-[0-9]\{2\}\)/\n\1/'"
        else
            grepCmd="${grepCmd} | sed 's/^Message:\([^|]*\)|/Message:\1\n/'"
        fi
    fi

    # 执行
    rc=1
    if [ "$canOptimized" == true ]; then
        # 重定向到指定文件
        echo "" > "${temp_results}.log"
        grepCmd="${grepCmd} > ${temp_results}.log"
        eval "${grepCmd}"
        rc=$?
    fi
    
    if [ $rc == 0 ]; then
        # 优化成功
        # 如果过滤的结果为空，表示没有匹配的日志内容，直接跳过
        line=`wc -l < "${temp_results}.log"`
        if [ "$line" == "0" ]; then
            continue
        fi
        file="${temp_results}.log"
    else
        # 没有优化或优化失败，使用原始文件
        if [ "$ORIGINAL_OUTPUT" != true ]; then
            # 简要模式需要带上 Message 的行号
            awk '
                # 拼接 message 行
                {
                    if (in_message) {
                        # 丢弃 Message: 后面多行内容，只打印一行
                        print "Message:" msg_line_num "\n" $0 "\n"
                        in_message = 0
                        next;
                    } else if ($0 == "Message:") {
                        # 标记 message 行
                        in_message = 1
                        msg_line_num = NR
                        next;
                    }

                    # 直接输出非 Message 的行
                    print
                }
            ' $file > "${temp_results}.log"
            test $? -ne 0 && echo "[ERROR] Failed to mark the line number of \"Message:\" in the log file \"$file\"" >&2 && exit 1
            file="${temp_results}.log"
        fi
    fi

    if [ "$FILES_ONLY" = true ]; then
        file_match_result=$(awk -v RS='\n\n' \
            -v level_num="$LEVEL_NUM" \
            -v pid="$PID" \
            -v tid="$TID" \
            -v msg_key="$MSG_KEYWORD" \
            -v error="$ERROR" \
            -v start_time="$START_TIMESTR" \
            -v end_time="$END_TIMESTR" \
            -v limit="$LIMIT" \
            -v current_total="$total_log_count" '
        BEGIN {
            matched = 0
            total_count = current_total
            level_names[0] = "SEVERE"
            level_names[1] = "ERROR"
            level_names[2] = "EVENT"
            level_names[3] = "WARNING"
            level_names[4] = "INFO"

            if (level_num != "") {
                level_pattern = "Level:[[:space:]]*("
                for (i = 0; i <= level_num; i++) {
                    if (i > 0) level_pattern = level_pattern "|"
                    level_pattern = level_pattern level_names[i]
                }
                level_pattern = level_pattern ")"
            }
        }
        
        {
            if ($1 !~ /^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]/) next
            
            # 时间范围检查
            if (end_time != "" && $1 > end_time) exit 0
            if (start_time != "" && $1 < start_time) next
            
            # 字段匹配检查
            block_matched = 1
            
            if (block_matched && level_num != "" && !($0 ~ level_pattern)) {
                block_matched = 0
            }
            
            if (block_matched && pid != "" && $0 !~ "PID:" pid " ") {
                block_matched = 0
            }
            
            if (block_matched && tid != "" && $0 !~ "TID:" tid "\n") {
                block_matched = 0
            }
            
            if (block_matched && (msg_key != "" || error != "")) {
                msg_found = 0
                if (match($0, /Message:/)) {
                    msg_start_pos = RSTART + RLENGTH
                    msg_content = substr($0, msg_start_pos)
                    gsub(/^[\n\r\t ]*/, "", msg_content)
                    if (msg_key != "" && error != "") {
                        if (msg_content ~ msg_key && msg_content ~ error "]?$") {
                            msg_found = 1
                        }
                    } else if (msg_key != "") {
                       if (msg_content ~ msg_key) {
                            msg_found = 1
                        }
                    } else if (error != " ") {
                        if (msg_content ~ error "]?$") {
                            msg_found = 1
                        }
                    }
                }
                block_matched = msg_found
            }
            
            if (block_matched) {
                matched++
                total_count++
                
                if (!limit || limit == 0) {
                    exit 0
                }
                else if (limit > 0 && total_count >= limit) {
                    exit 0
                }
            }
        }
        
        END {
            print matched
            exit (matched > 0 ? 0 : 1)
        }
        ' "$file")
        
        if [ $? -eq 0 ]; then
            file_match_count=$(echo "$file_match_result")
            if [[ ! -z "${HOST_NAME}" && ! -z "${SERVICE_NAME}" && ! -z "${NODE_ROLE}" ]]; then
                matched_files+=("${HOST_NAME}@@${SERVICE_NAME}@@${real_file}")
            else
                matched_files+=("${real_file}")
            fi
            total_matched=$((total_matched + 1))
            total_log_count=$((total_log_count + file_match_count))
            
            if [ -n "$LIMIT" ] && [ "$total_log_count" -ge "$LIMIT" ]; then
                echo "[INFO] Logs have been found ($total_log_count >= $LIMIT), stop searching"
                break
            fi
        fi
    else
        awk -v RS='\n\n' -v ORS='\n\n' \
            -v level_num="$LEVEL_NUM" \
            -v pid="$PID" \
            -v tid="$TID" \
            -v msg_key="$MSG_KEYWORD" \
            -v error="$ERROR" \
            -v start_time="$START_TIMESTR" \
            -v end_time="$END_TIMESTR" \
            -v context_before="$CONTEXT_BEFORE" \
            -v context_after="$CONTEXT_AFTER" \
            -v limit="$LIMIT" \
            -v original_output="$ORIGINAL_OUTPUT" \
            -v real_file="$real_file" \
            -v node_name="${HOST_NAME}:${SERVICE_NAME}" \
            -v role="$NODE_ROLE" \
            -v isLimit="$isLimit" '
        BEGIN {
            matched = 0
            record_count = 0
            level_names[0] = "SEVERE"
            level_names[1] = "ERROR"
            level_names[2] = "EVENT"
            level_names[3] = "WARNING"
            level_names[4] = "INFO"

            if (level_num != "") {
                level_pattern = "Level:("
                for (i = 0; i <= level_num; i++) {
                    if (i > 0) level_pattern = level_pattern "|"
                    level_pattern = level_pattern level_names[i]
                }
                level_pattern = level_pattern ")"
            }
        }
        
        {
            # 时间行匹配
            if ($1 !~ /^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]/) next
            
            # 时间范围检查
            if (end_time != "" && $1 > end_time) exit 0
            if (start_time != "" && $1 < start_time) {
                if (original_output) {
                    records[++record_count] = ""
                    record_times[record_count] = ""
                }
            } else {
                records[++record_count] = $0
                if (!original_output) {
                    record_times[record_count] = $1
                }
            }
        }
        
        END {
            for (i = 1; i <= record_count; i++) {
                current_record = records[i]
                if (current_record == "") continue

                # 字段匹配检查
                block_matched = 1

                if (block_matched && tid != "" && current_record !~ "TID:" tid "\n") {
                    block_matched = 0
                }

                if (block_matched && pid != "" && current_record !~ "PID:" pid " ") {
                    block_matched = 0
                }
                
                if (block_matched && level_num != "" && !(current_record ~ level_pattern)) {
                    block_matched = 0
                }

                if (block_matched && (msg_key != "" || error != "")) {
                    msg_found = 0
                    if (match(current_record, /Message:/)) {
                        msg_start_pos = RSTART + RLENGTH
                        msg_content = substr(current_record, msg_start_pos)
                        gsub(/^[\n\r\t ]*/, "", msg_content)
                        if (msg_key != "" && error != "") {
                            if (msg_content ~ msg_key && msg_content ~ error "]?$") {
                                msg_found = 1
                                if (!original_output) msg[i] = msg_content
                            }
                        } else if (msg_key != "") {
                            if (msg_content ~ msg_key) {
                                msg_found = 1
                                if (!original_output) msg[i] = msg_content
                            }
                        } else if (error != "") {
                            if (msg_content ~ error "]?$") {
                                msg_found = 1
                                if (!original_output) msg[i] = msg_content
                            }
                        }
                    }
                    block_matched = msg_found
                } else if (block_matched) {
                    if (match(current_record, /Message:/)) {
                        msg_start_pos = RSTART + RLENGTH
                        msg_content = substr(current_record, msg_start_pos)
                        gsub(/^[\n\r\t ]*/, "", msg_content)
                        if (!original_output) msg[i] = msg_content
                    }
                }
                if (block_matched) {
                    matched_records[i] = 1
                }
            }

            for (i = record_count; i >= 1; i--) {
                if (matched_records[i]) {
                    if (context_after > 0) {
                        end_idx = (i + context_after <= record_count) ? (i + context_after) : record_count
                        for (j = i + 1; j <= end_idx; j++) {
                            if (!matched_records[j]) {
                                block_content = records[j]
                                if (original_output) {
                                    gsub(/\n/, "\\n", block_content)
                                    if (node_name != ":") {
                                        printf "===============%s===============\\n%s\n", node_name, block_content
                                    } else {
                                        printf "%s\n", block_content
                                    }
                                } else {
                                    match_time = record_times[j]
                                    match(block_content, /Message:[0-9]+\n/)
                                    msg_content = substr(block_content, RSTART + RLENGTH)
                                    match(block_content, /Message:([0-9]+)\n/, arr)
                                    match_line = arr[1] + 1
                                    if (node_name != ":") {
                                        printf "%s,%s,%s:%d,%s,\"%s\"\n", node_name, role, real_file, match_line, match_time, msg_content
                                    } else {
                                        printf "<localhost:port>,<node>,%s:%d,%s,\"%s\"\n", real_file, match_line, match_time, msg_content
                                    }
                                }
                            }
                        }
                    }
  
                    if (original_output) {
                        block_content = records[i]
                        gsub(/\n/, "\\n", block_content)
                        if (node_name != ":") {
                            printf "===============%s===============\\n%s\n", node_name, block_content
                        } else {
                            printf "%s\n", block_content
                        }
                    } else {
                        match_time = record_times[i]
                        split(msg[i], parts, "\n")
                        match_line = parts[1] + 1
                        match_msg = parts[2]
                        if (node_name != ":") {
                            printf "%s,%s,%s:%d,%s,\"%s\"\n", node_name, role, real_file, match_line, match_time, match_msg
                        } else {
                            printf "<localhost:port>,<node>,%s:%d,%s,\"%s\"\n", real_file, match_line, match_time, match_msg
                        }
                    }
                    matched++
                    
                    if (context_before > 0) {
                        start_idx = (i - context_before > 0) ? (i - context_before) : 1
                        for (j = start_idx; j < i; j++) {
                            if (!matched_records[j]) {
                                block_content = records[j]
                                if (original_output) {
                                    gsub(/\n/, "\\n", block_content)
                                    if (node_name != ":") {
                                        printf "===============%s===============\\n%s\n", node_name, block_content
                                    } else {
                                        printf "%s\n", block_content
                                    }
                                } else {
                                    match_time = record_times[j]
                                    match(block_content, /Message:[0-9]+\n/)
                                    msg_content = substr(block_content, RSTART + RLENGTH)
                                    match(block_content, /Message:([0-9]+)\n/, arr)
                                    match_line = arr[1] + 1
                                    if (node_name != ":") {
                                        printf "%s,%s,%s:%d,%s,\"%s\"\n", node_name, role, real_file, match_line, match_time, msg_content
                                    } else {
                                        printf "<localhost:port>,<node>,%s:%d,%s,\"%s\"\n", real_file, match_line, match_time, msg_content
                                    }
                                }
                            }
                        }
                    }
                    
                    if (limit > 0 && matched >= limit) {
                        break
                    }
                }
            }
        }
        ' "$file" >> "$temp_results" 2>/dev/null
        
        if [ -n "$LIMIT" ]; then
            current_count=$(wc -l < "$temp_results")
            if [ "$current_count" -ge "$LIMIT" ]; then
                echo "[INFO] Logs have been found ($current_count >= $LIMIT), stop searching next file"
                break
            fi
        fi
    fi
done

if [ "$FILES_ONLY" = true ]; then
    if [ ${#matched_files[@]} -gt 0 ]; then
        if [ -n "$OUTPUT_FILE" ]; then
            printf '%s\n' "${matched_files[@]}" > "$OUTPUT_FILE"
            echo "[INFO] The result of matching files are saved to: \"$OUTPUT_FILE\""
        else
            printf '%s\n' "${matched_files[@]}"
        fi
        echo "[INFO] Number of matching files: ${#matched_files[@]}"
    else
        if [ -n "$OUTPUT_FILE" ]; then
            touch "$OUTPUT_FILE"
            echo "" > "$OUTPUT_FILE"
        fi
        echo "[INFO] No logs matched" >&2
    fi
else
    if [ -s "$temp_results" ]; then
        if [ -n "$LIMIT" ]; then
            head -n $((LIMIT)) "$temp_results" > "$temp_results.limited"
        else
            mv "$temp_results"  "$temp_results.limited"
        fi

        # 上面从日志搜索结果已经有序，无需额外排序
        if [ "$ORIGINAL_OUTPUT" == true ]; then
            # sed 去掉头尾多余的换行
            sed 's/\\n$//' "$temp_results.limited" | awk '{
                content=$0
                gsub(/(\\n)|(\\n\\n)/, "\n", content)
                print content "\n"
            }' > "$temp_results.sorted"
        else
            mv "$temp_results.limited" "$temp_results.sorted"
        fi

        if [ "$ORIGINAL_OUTPUT" == true ] ;then
            match_count=$(grep -ac '^[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\}-[0-9]\{2\}' "$temp_results.sorted")
        else
            if [[ -z "${HOST_NAME}" && -z "${SERVICE_NAME}" && -z "${NODE_ROLE}" ]]; then
                sed -i 's#^<localhost:port>,<node>,##g' "$temp_results.sorted"
            fi
            match_count=$(($(wc -l < "$temp_results.sorted")))
        fi
        echo "[INFO] Number of matching logs: $match_count"

        if [ -n "$OUTPUT_FILE" ]; then
            mv "$temp_results.sorted" "$OUTPUT_FILE"
            echo "[INFO] The matching logs are saved to: $OUTPUT_FILE"
        else
            cat "$temp_results.sorted"
        fi
    else
        echo "[INFO] No logs matched" >&2
        if [ -n "$OUTPUT_FILE" ]; then
            # 创建空文件以标识完成搜索
            touch "$OUTPUT_FILE"
            test $? -ne 0 && echo "[ERROR] Failed to touch \"$OUTPUT_FILE\"" >&2 && exit 1
        fi
    fi
fi

# 清理临时文件
rm -f "$temp_results" "$temp_results.sorted" "$temp_results.limited" "${temp_results}.log"
exit 0
