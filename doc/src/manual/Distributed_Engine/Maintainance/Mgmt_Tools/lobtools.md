[^_^]:
    大对象工具
    作者：赵育
    时间：20190305
    评审意见
    王涛：20190308
    许建辉：
    市场部：20190521
    

sdblobtool 是 SequoiaDB 巨杉数据库的[大对象][lob]管理工具，用于导入、导出和迁移大对象数据。该工具在使用过程中，会在当前执行路径下生成日志文件 `sdblobtool.log`，以保存工具的运行日志。

##语法规则##

**sdblobtool <--operation arg> <--collection arg> [--hostname arg] [--svcname arg] [--usrname arg] [--passwd arg] [--file arg] [--dstcollection arg] [--dsthost arg] [--dstservice arg] [--dstusrname arg] [--dstpasswd arg] [--ignorefe arg] [--prefer arg] [--ssl arg] [--token arg] [--cipher arg] [--cipherfile arg]**

##参数说明##

| 参数名          | 缩写 | 描述 | 是否必填 |
| ----            | ---- | ---- | -------- |
| --help          | -h   | 获取帮助信息 | 否 |
| --version       | -v   | 获取版本信息 | 否 |
| --operation     | -    | 指定操作类型，取值如下：<br> ● export：将集合中的大对象导出至文件 <br> ● import：将文件中的大对象导入至集合 <br>● migration：将集合中的大对象复制到其他集合<br>该参数取值为 migration 时，必须指定参数 --dstcollection | 是 |
| --collection    | -    | 指定集合名  | 是 |
| --hostname      | -    | 指定协调节点地址，默认值为 localhost | 否 |
| --svcname       | -    | 指定协调节点端口号，默认值为 11810  | 否 |
| --usrname       | -    | 指定数据库用户名<br>当数据库开启鉴权时，需指定该参数 | 否 |
| --passwd        | -    | 指定数据库用户的密码  | 否 |
| --file          | -    | 指定大对象文件的全路径<br>该参数在 --operation 取值为 export 或 import 时有效 | 否 |
| --dstcollection | -    | 指定目标数据库的集合名<br>该参数在 --operation 取值为 migration 时有效 | 否 |
| --dsthost       | -    | 指定目标数据库的协调节点地<br>该参数在 --operation 取值为 migration 时有效 | 否 |
| --dstservice    | -    | 指定目标数据库的协调节点端口号<br>该参数在 --operation 取值为 migration 时有效 | 否 |
| --dstusrname    | -    | 指定目标数据库的用户名<br>该参数在 --operation 取值为 migration 时有效  | 否 |
| --dstpasswd     | -    | 指定目标数据库的密码<br>该参数在 --operation 取值为 migration 时有效 | 否 |
| --ignorefe      | -    | 指定是否导入目标集合中已存在的大对象，默认值为 false，表示不导入目标集合中已存在的大对象<br>该参数在 --operation 取值为 import 或 migration 时有效 | 否 |
| --prefer        | -    | 指定优先选择的实例，默认值为 M，取值如下：<br>● m 或 M：master<br>● s 或 S：slave<br>● a 或 A：anyone<br>● 1~7：node1~node7<br>该参数在 --operation 取值为 export 时有效 | 否 |
| --ssl           | -    | 指定是否使用 SSL 连接，默认值为 false，表示不使用 SSL 连接 | 否 |
| --token         | -    | 指定密文文件的加密令牌<br>如果创建密文文件时未指定参数 token，可忽略该参数 | 否 |
| --cipher        | -    | 指定是否使用密文文件输入密码，默认值为 false，表示不使用密文文件 | 否 |
| --cipherfile    | -    | 指定密文文件，默认值为 `~/sequoiadb/passwd`<br>该参数在 --cipher 取值为 true 时有效 | 否 |

##常见场景##

- 将集合 sample.employee 中的大对象导出至 `/opt/mylob` 文件中

   ```lang-bash
   $ sdblobtool --operation export --hostname localhost --svcname 11810 --collection sample.employee --file /opt/mylob
   ```

- 将大对象文件 `/opt/mylob` 导入到集合 sample.employee 中，并指定导入时跳过重复记录

   ```lang-bash
   $ sdblobtool --operation import --hostname localhost --svcname 11810 --collection sample.employee --file /opt/mylob --ignorefe
   ```
- 将 sdbserver1 的集合 sample.employee 中所存储的大对象复制到 sdbserver2 的集合 sample.employee 中

   ```lang-bash
   $ sdblobtool --operation migration --hostname sdbserver1 --svcname 11810 --collection sample.employee --dsthost sdbserver2 --dstservice 11810 --dstcollection sample.employee
   ```
  
[^_^]:
    本文使用到的所有链接及引用
[lob]:manual/Distributed_Engine/Architecture/Data_Model/lob.md
    
    
