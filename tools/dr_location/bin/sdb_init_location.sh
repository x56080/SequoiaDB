#!/bin/bash
# ========================================
# sdb_init_location.sh - 初始化和展示 Location 信息
# ========================================

# 脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 显示帮助
function show_help() {
    cat << EOF
Usage:
    sdb_init_location.sh [command] [options]

Commands:
    show          Show current cluster location information
    check         Check location configuration against expected
    init          Initialize location configuration

Options:
    -h, --help          Show help message
    -c, --conf <file>   Specify config file, default config/config.js
    -f, --file <file>   Specify location file in check or init mode

Examples:
    sdb_init_location.sh
    sdb_init_location.sh show
    sdb_init_location.sh check
    sdb_init_location.sh init

Others:
    -f, --file <file> content is as follows:
        [GuangZhou(active)]
        host1
        host2

        [ShenZhen]
        host3
EOF
}

# 主函数
function main() {
    local command=""
    local configJs=""
    local file=""
    local params=""

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
            -f|--file)
                file="$2"
                shift 2
                ;;
            show)
                command="show"
                shift
                ;;
            check)
                command="check"
                shift
                ;;
            init)
                command="init"
                shift
                ;;
            *)
                echo "Unknown option: $1"
                show_help
                return 1
                ;;
        esac
    done

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

    # 检查配置文件
    if [[ "" == "$configJs" && ! -f "$PROJECT_ROOT/config/config.js" ]]; then
        echo "[ERROR] Config file not found: $PROJECT_ROOT/config/config.js, please run \"cp $PROJECT_ROOT/config/config.js.sample $PROJECT_ROOT/config/config.js\""
        return 1
    fi

    # 构建参数列表，用 ; 分隔
    local cmd="var scriptDir=\"$SCRIPT_DIR\""
    if [ -n "$command" ]; then
        cmd="$cmd; var mode=\"$command\""
    else
        cmd="$cmd; var mode=\"show\""
    fi
    
    cmd="$cmd; var projectRoot=\"$PROJECT_ROOT\""
    if [ -n "$configJs" ]; then
        cmd="$cmd; var configJs=\"$configJs\""
    fi
    if [ -n "$file" ]; then
        if [ "$command" == "show" ]; then
            echo "[ERROR] -f, --file can only use in check or init mode"
            return 1
        fi
        cmd="$cmd; var locationFile=\"$file\""
    fi

    $sdb_shell -f "$PROJECT_ROOT/bin/main.js" -e "$cmd"
    return $?
}

main "$@"
exit $?
