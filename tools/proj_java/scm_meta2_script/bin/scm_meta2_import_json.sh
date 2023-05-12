#!/bin/bash

while getopts ":t:s:f:u:w:" opt; do
  case ${opt} in
    t )
      type_value=$OPTARG
      ;;
    s )
      host_value=$OPTARG
      ;;
    f )
      filepath_value=$OPTARG
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
if [ -z "$type_value" ] || [ -z "$host_value" ] || [ -z "$filepath_value" ] || [ -z "$user_value" ] || [ -z "$passwd_value" ]; then
  echo "Error: Missing required argument."
  echo "Argument Format: -s <hostname:port> -u <user> -w <password> -t <json type> -f <json file>"
  exit 1
fi

# 从 IBSFLOW.FILE_2020.meta.json（相对路径或者绝对路径） 获取元数据表名
filename=`echo ${filepath_value} | awk -F '/' '{print $(NF)}'`
csname=`echo ${filename} | awk -F '.' '{print $(NF-3)}'`
clname=`echo ${filename} | awk -F '.' '{print $(NF-2)}'`
jsontype=`echo ${filename} | awk -F '.' '{print $(NF-1)}'`

# 检测是否输入错误的 json 文件
if [ ${jsontype} !=  ${type_value} ]; then
  echo "Error: the json file does not match -t option, please check." 1>&2
  exit 1
fi

# 从输入的 json 文件名中，解析出用于导入的 cs/cl 名
if [ ${type_value} = "meta" ]; then
  newcsname="${csname}_test"
  newclname="${clname}_meta"
elif [ ${type_value} = "lobinfo" ]; then
  newcsname="${csname}_test"
  newclname="${clname}_lobinfo"
else
  echo "Error: Option -t requires an argument: meta or lobinfo." 1>&2
  exit 1
fi

echo "Import info:"
echo "host      = $host_value"
echo "type      = $type_value"
echo "filepath  = $filepath_value"
echo "import cs = $newcsname"
echo "import cl = $newclname"
echo ""

# 导入数据
/opt/sequoiadb/bin/sdbimprt --hosts ${host_value} -u ${user_value} -w ${passwd_value} --type json --file ${filepath_value} --csname ${newcsname} --clname ${newclname}

# 判断执行是否成功
if [ $? = 0 ]; then
  # 不打印成功，因为 sdbimprt 部分场景失败时，$? 也返回 0，已提单处理
  # echo "Succeed to import!"
  exit 0
else
  echo "Failed to import!"
  exit 1
fi