#!/bin/bash
# ========================================
# sdb_start_critical.sh - 开启 CriticalMode
# ========================================

# 脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 显示帮助
function show_help() {
    cat << EOF
Usage:
    sdb_start_critical.sh [command] [options]

Commands:
    start_critical    Start CriticalMode

Options:
    -h, --help                 Show help message
    -c, --conf <file>          Specify config file, default config/config.js
    -l, --location <location>  Location(s) with the specified CriticalMode enabled, cannot be used with -H,--hostname
    -H, --hostname <host>      Hostname(s) with the specified CriticalMode enabled, cannot be used with -l,--location
    -d, --domain <domain>      Filter the data groups within the specify domain(s)
    -f, --file <file>          Specify node information file, ignore other command-line node args

Examples:
    sdb_start_critical.sh -l GuangZhou
    sdb_start_critical.sh -H host1,host2
    sdb_start_critical.sh -l GuangZhou -d domain1
    sdb_start_critical.sh -f target

Notes:
    CriticalMode will set these nodes to be primary during re-election
    This may cause data rollback if not used properly
    Use with caution and follow proper DR procedures

Others:
    -f, --file <file> content is as follows:
        [location]
        location1
        location2

        [hostname]
        host1
        host2

        [doamin]
        domain1
        domain2
EOF
}

# 主函数
function main() {
    local configJs=""
    local locations=""
    local hostnames=""
    local domains=""
    local file=""

    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--conf)
                configJs="$2"
                shift 2
                ;;
            -l|--location)
                locations="$2"
                shift 2
                ;;
            -H|--hostname)
                hostnames="$2"
                shift 2
                ;;
            -d|--domain)
                domains="$2"
                shift 2
                ;;
            -f|--file)
                file="$2"
                shift 2
                ;;
            *)
                echo "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done

    # 检查配置文件
    if [[ "" == "$configJs" && ! -f "$PROJECT_ROOT/config/config.js" ]]; then
        echo "[ERROR] Config file not found: $PROJECT_ROOT/config/config.js, please run \"cp $PROJECT_ROOT/config/config.js.sample $PROJECT_ROOT/config/config.js\""
        return 1
    fi

    # 不能同时指定 -l,--location 和 -H,--hostname
    if [[ "" != "$locations" && "" != "$hostnames" ]]; then
        echo "[ERROR] Parameter \"-l,--location\" and \"-H,--hostname\" cannot be used at the same time"
        return 1
    fi

    # 检查 sdb 安装目录
    if [ ! -f /etc/default/sequoiadb ]; then
        echo "[ERROR] /etc/default/sequoiadb not found"
        return 1
    fi

    . /etc/default/sequoiadb
    sdb_shell="$INSTALL_DIR/bin/sdb"
    if [ ! -f "$sdb_shell" ]; then
        echo "[ERROR] $sdb_shell not found"
        return 1
    fi

    SCRIPT_DIR=$(cd "$(dirname $0)" && pwd)/../
    test $? -ne 0 && echo "[ERROR] failed to get script dir from $0" && return 1

    # 构建参数列表，用 ; 分隔
    local cmd="var scriptDir=\"$SCRIPT_DIR\"; var mode=\"start_critical\""
    cmd="$cmd; var projectRoot=\"$PROJECT_ROOT\"; var configJs=\"$configJs\""

    if [ -n "$file" ]; then
        # 忽略其他命令行配置
        cmd="$cmd; var tagertFile=\"$file\""
    else
        if [ -n "$locations" ]; then
            locationArray=(`echo "$locations" | sed 's/,/ /g'`)
            locationStr="\"${locationArray[0]}\""
            for((i=1; i<${#locationArray}; i++))
            do
                test "${locationArray[i]}" != "" && locationStr="$locationStr,\"${locationArray[i]}\""
            done
            cmd="$cmd; var locations=[$locationStr]"
        fi
        if [ -n "$hostnames" ]; then
            hostnameArray=(`echo "$hostnames" | sed 's/,/ /g'`)
            hostnameStr="\"${hostnameArray[0]}\""
            for((i=1; i<${#hostnameArray}; i++))
            do
                test "${hostnameArray[i]}" != "" && hostnameStr="$hostnameStr,\"${hostnameArray[i]}\""
            done
            cmd="$cmd; var hostnames=[$hostnameStr]"
        fi
        if [ -n "$domains" ]; then
            domainArray=(`echo "$domains" | sed 's/,/ /g'`)
            domainStr="\"${domainArray[0]}\""
            for((i=1; i<${#domainArray}; i++))
            do
                test "${domainArray[i]}" != "" && domainStr="$domainStr,\"${domainArray[i]}\""
            done
            cmd="$cmd; var domains=[$domainStr]"
        fi
    fi

    $sdb_shell -f "$PROJECT_ROOT/bin/main.js" -e "$cmd"
    return $?
}

main "$@"
exit $?
