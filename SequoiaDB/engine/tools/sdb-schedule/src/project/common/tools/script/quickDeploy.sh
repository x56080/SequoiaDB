#!/bin/bash
set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
BASE_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)
CONF_FILE="${SCRIPT_DIR}/quickDeploy.conf"

ACTION="$1"

usage() {
cat <<EOF
用法:
  ./quickDeploy.sh <action>

action:
  deploy       创建节点、创建数据源站点
  createnode   只创建调度节点（并启动服务）
  createsite   只创建数据源站点
  help|-h      显示帮助
EOF
}

if [ -z "${ACTION}" ] || [[ "${ACTION}" == "help" || "${ACTION}" == "-h" || "${ACTION}" == "--help" ]]; then
    usage
    exit 0
fi

if [ ! -d "${BASE_DIR}/bin" ]; then
    echo "错误：请在解压后的 sdb-schedule/script 目录下执行本脚本"
    exit 1
fi

if [ ! -f "${CONF_FILE}" ]; then
    echo "错误：配置文件不存在：${CONF_FILE}"
    exit 1
fi

source "${CONF_FILE}"

CURRENT_USER=$(whoami)
BASE_DIR_OWNER=$(stat -c %U "${BASE_DIR}")

if [ "${CURRENT_USER}" != "${BASE_DIR_OWNER}" ]; then
    echo "错误：当前执行用户      ：${CURRENT_USER}"
    echo "      安装目录所属用户  ：${BASE_DIR_OWNER}"
    echo ""
    echo "请使用用户 ${BASE_DIR_OWNER} 执行本脚本"
    exit 1
fi

echo ">>> 配置信息"
echo "安装目录              ：${BASE_DIR}"
echo "安装目录所属用户       ：${BASE_DIR_OWNER}"
echo "RootSite              ：${rootsite_url}"
echo "节点端口              ：${node_list}"
echo "数据源列表            ：${datasource_list}"
echo "系统集合空间名         ：${system_cs_name}"
echo "集合空间存储数据域     ：${system_cs_domain}"
echo ""

read -p "确认开始执行？[Y/n] " confirm
confirm=${confirm:-y}

if [[ ! "${confirm}" =~ ^[yY]$ ]]; then
    echo "操作已取消"
    exit 0
fi

create_nodes() {
    echo ""
    echo ">>> 创建调度服务节点"

    IFS=',' read -ra NODE_ARRAY <<< "${node_list}"

    for port in "${NODE_ARRAY[@]}"; do
        port=$(echo "${port}" | xargs)

        echo "创建节点：${port}"

        if [ -d "${BASE_DIR}/conf/schedule-server/${port}" ]; then
            echo "节点 ${port} 已存在，跳过"
            continue
        fi

        if [ -z "${system_cs_domain}" ]; then
            "${BASE_DIR}/bin/schadmin.sh" createnode \
                -p "${port}" \
                --ms-url "${rootsite_url}" \
                --ms-user "${rootsite_user}" \
                --ms-passwd "${rootsite_password}" \
                --systemCsName "${system_cs_name}"
        else
            "${BASE_DIR}/bin/schadmin.sh" createnode \
                -p "${port}" \
                --ms-url "${rootsite_url}" \
                --ms-user "${rootsite_user}" \
                --ms-passwd "${rootsite_password}" \
                --systemCsName "${system_cs_name}" \
                --systemCsStoreDomain "${system_cs_domain}"
        fi

        echo "节点 ${port} 创建完成"
    done
}

start_service() {
    echo ""
    echo ">>> 启动调度服务"
    "${BASE_DIR}/bin/schctl.sh" start -t all
}

create_sites() {
    echo ""
    echo ">>> 创建数据源站点"

    IFS=',' read -ra DS_ARRAY <<< "${datasource_list}"

    for ds in "${DS_ARRAY[@]}"; do
        ds=$(echo "${ds}" | xargs)
        site_name="${ds}-site"

        echo "创建数据源站点, 站点名：${site_name}， 数据源名：${ds}"

        "${BASE_DIR}/bin/schadmin.sh" createsite \
            --name "${site_name}" \
            --ms-url "${rootsite_url}" \
            --ms-user "${rootsite_user}" \
            --ms-passwd "${rootsite_password}" \
            --systemCsName "${system_cs_name}" \
            --ds-name "${ds}"

        echo "站点 ${site_name} 创建完成"
    done
}

case "${ACTION}" in
    deploy)
        create_nodes
        start_service
        create_sites
        ;;
    createnode)
        create_nodes
        start_service
        ;;
    createsite)
        create_sites
        ;;
    *)
        echo "未知参数：${ACTION}"
        usage
        exit 1
        ;;
esac
