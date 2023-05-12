#!/bin/bash

while getopts ":s:c:o:u:w:" opt; do
  case ${opt} in
    s )
      host_value=$OPTARG
      ;;
    c )
      table_value=$OPTARG
      ;;
    o )
      dir_value=$OPTARG
      ;;
    u )
      user_value=$OPTARG
      ;;
    w )
      passwd_value=$OPTARG
      ;;
    \? )
      echo "Error: Invalid option: -$OPTARG" 1>&2
      exit 1
      ;;
    : )
      echo "Error: Option -$OPTARG requires an argument." 1>&2
      exit 1
      ;;
  esac
done

# 检查必填参数是否存在
if [ -z "$host_value" ] || [ -z "$table_value" ] || [ -z "$dir_value" ] || [ -z "$user_value" ] || [ -z "$passwd_value" ]; then
  echo "Error: Missing required argument."
  echo "Argument Format: -s <hostname:port> -u <user> -w <password> -c <meta table name> -o <output dir>"
  exit 1
fi

# 构建输出文件路径
filepath=${dir_value}"/"${table_value}".update.json"

# 从输入的元数据表名中，解析出用于导出的 cs/cl 名
csname=`echo ${table_value} | awk -F '.' '{print $1}'`
clname=`echo ${table_value} | awk -F '.' '{print $2}'`
newcsname="${csname}_test"
newclname="${clname}_update"

echo "Export info:"
echo "host        = $host_value"
echo "output path = $filepath"
echo "export cs   = $newcsname"
echo "export cl   = $newclname"
echo ""

# 导出数据
/opt/sequoiadb/bin/sdbexprt --strict true --hosts ${host_value} -u ${user_value} -w ${passwd_value} --type json --file ${filepath} --csname ${newcsname} --clname ${newclname}

# 判断执行是否成功
if [ $? = 0 ]; then
  echo "Succeed to export!"
  exit 0
else
  echo "Failed to export!"
  exit 1
fi