归档的日志可以通过日志重放工具 sdbreplay 在其它集群或节点重新执行。

##基本功能##

日志重放工具主要功能：

* 读取日志并在 SequoiaDB 上执行；
* 可以根据条件过滤日志（日志文件、集合空间、集合、操作、LSN）；
* 支持持续监控归档目录并重放日志；
* 支持在后台执行；
* 可以生成状态文件从而在退出重启后接续执行退出前的重放。

目前重放工具支持重放的日志操作如下：

|操作|说明|
|---|---|
|insert|插入数据|
|update|更新数据|
|delete|删除数据|
|truncatecl|truncate集合|

> Note:  
>
> * 重放时根据归档文件的 FileId 从小到大重放；  
> * 重放出错时重放工具会立即退出，已经重放的日志不会回滚；  
> * 归档日志 LSN 不连续时重放工具会报错退出；  
> * 通过 status 选项指定状态文件，可以从上次退出的地方继续重放，不指定时每次从第一个归档文件开始重放；  
> * 未重放的归档日志文件发生了移动操作后，移动文件不会被重放；已重放的归档日志文件发生了移动操作，重放工具会回滚移动文件中的日志操作；  
> * 不支持回滚 truncatecl 操作，回滚时遇到 truncatecl 操作会报错退出；  
> * 在后台执行时可以通过 kill -15 \<pid\> 的方式停止进程。  
> * 复制日志是幂等的，同一条日志多次重放结果不变。  
> * 归档重放过程中，如果在集合上执行 split 操作，通过协调节点重放到同一个集群时可能会丢失数据。因为 split 的源复制组在数据迁移到目标复制组后会删除本地相应数据，并生成删除日志；而目标复制组接收数据后生成插入日志。如果目标复制组的归档日志先被重放而后源复制组的归档日志才被重放，那么迁移的那部分数据重放时先插入后被删除，导致数据丢失。此时可以通过再次重放目标复制组的归档日志来重新插入丢失的数据。

##命令选项##

|选项|缩写|描述|类型|默认值|说明|
|---|---|---|---|---|---|
|help|h|打印帮助信息|-|-|
|version|V|打印版本号|-|-|
|hostname|-|SequoiaDB 所在的主机名|String|-|dump 和 dumpheader 为 false，且 outputconf 未设置时必填|
|svcname|-|SequoiaDB 的服务名（端口号）|String|-|dump 和 dumpheader 为 false，且 outputconf 未设置时时必填|
|user|-|数据库用户|String|-|
|password|-|数据库用户密码|String|-|如果不使用该参数指定密码，工具会通过交互式界面提示用户输入密码|
|cipher|-|是否使用密文模式输入密码。关于密文模式的介绍，详细可参考[密码管理](database_management/security/system_security.md)|Bool|false，不使用密文模式输入密码|
|token|-|加密令牌|String|-|
|cipherfile|-|密文文件路径|String|`~/sequoiadb/passwd`|
|ssl|-|使用 SSL 连接|Bool|false，不使用 SSL 连接|
|path|-|归档目录|String|-|必填，可以是文件或目录|
|outputconf|-|输出格式的配置文件路径|String|-|用于配置回放工具的输出规则，当 hostname 设置时，该参数无效。详细说明参见下一小节|
|filter|-|过滤条件|String(json)|-|
|dump|-|导出日志后是否重放。|Bool|false，不重放|
|dumpheader|-|导出归档文件头后是否重放|Bool|false，不重放|
|delete|-|重放后是否删除完成重放的归档日志文件|Bool|false，不删除归档文件|只删除完整归档文件|
|watch|-|是否持续监控归档目录并重放日志|Bool|false，不持续监控|path为目录时有效|
|daemon|-|是否在后台运行|Bool|false，不在后台运行|kill -15 \<pid\> 可以使后台进程正确退出|
|status|-|指定状态文件|String|-|状态文件会存储重放的状态信息，首次指定时重放工具会生成该文件。重放工具退出后，通过指定状态文件可以从上次退出的地方继续重放。|
|intervalnum|-|状态文件持久化间隔记录数|Int|1000|每回放 intervalnum 条记录持久化一次状态文件|
|type|-|指定日志类型|String|archive|取值为 "archive" 表示归档日志，取值为 "replica" 表示复制日志。|

其中 filter 是 json 格式的字符串，可以指定过滤条件对日志进行过滤，过滤条件有：

|字段|类型|说明|
|---|---|---|
|OP|Array[string]|指定重放的操作，默认为重放工具支持的所有日志操作|
|ExclOP|Array[string]|指定排除的操作，优先级高于 OP|
|MinLSN|Int64|指定重放的最小 LSN（包括该 LSN）|
|MaxLSN|Int64|指定重放的最大 LSN（不包括该 LSN）|
|File|Array[string]|指定重放的日志文件，默认为全部|
|ExclFile|Array[string]|指定排除的日志文件，优先级高于 File|
|CS|array[String]|指定重放的集合空间，默认为全部|
|ExclCS|Array[string]|指定排除的集合空间，优先级高于 CS|
|CL|Array[string]|指定重放的集合，集合名的格式为"集合空间.集合名"，默认为全部|
|ExclCL|Array[string]|指定排除的集合，优先级高于 CL|

> Note:  

> * 命令选项 watch 在 path 指定为目录时有效；  
> * dump 或 dumpheader 为 true 不需要设置 hostname 和 svcname。

##outputconf说明##

outputconf 是以 json 格式表示的，用于设置输出格式的配置文件。当前只支持一种输出类型：DB2LOAD。其具体参数有：

|参数|描述|类型|说明|
|----|----|----|----|
|outputType|目标格式类型|String|当前只有一种类型：DB2LOAD|
|outputDir|输出目录|String|-|
|filePrefix|输出结果文件名的前缀|String|-|
|fileSuffix|输出结果文件名的后缀|String|与 filePrefix 至少配置一个。该后缀在文件类型前面|
|delimiter|字段分隔符|String|默认为','|
|submitTime|文件提交时间|String|格式：21:00 。21 点提交一次正式文件|
|submitInterval|文件提交的时间间隔|Long|单位：分钟，每隔指定的分钟数提交一次文件。当配置了 submitInterval 后，submitTime 不生效|
|tables|表映射关系|Array[obj]|-|
|tables.source|源表名|String|-|
|tables.target|目标表名|String|-|
|fields|字段映射关系|Array[obj]|-|
|fields.fieldType|字段类型|String|目前支持：ORIGINAL_TIME（复制日志生成时间）,AUTO_OP（操作标识串。I:insert, D:delete, B:before update, A:after update）,CONST_STRING（常量字符串）, MAPPING_STRING（映射原始记录中的字符串），MAPPING_INT（映射原始记录中的int类型字段），MAPPING_LONG（映射原始记录中的long类型字段），MAPPING_DECIMAL（映射原始记录中的decimal类型字段），MAPPING_TIMESTAMP（映射原始记录中的时间戳字段）|
|fields.doubleQuote|value 是否带双引号|Bool|true，value 带双引号|
|fields.constValue|fieldType 为 CONST_STRING 时使用，表示常量的值|String|-|
|fields.source|源字段名|String|-|
|fields.target|目标字段名|String|-|
|fields.defaultValue|默认值|String|当源字段名不存在时，输出默认值，而不是报错。仅支持映射字段（以 MAPPING_ 开头的字段类型）|

outputconf具体样例：

```lang-json
$cat output.conf
{
   outputType: "DB2LOAD",
   outputDir: "/home/mount/sequoiadb/replay/output/",
   filePrefix: "SDB_db1_1000",
   submitTime: "21:00",
   delimiter: ",",
   tables:
   [
      {
         source: "cs.cl",
         target: "dbName.tableName",
         fields:
         [
            {
               fieldType: "ORIGINAL_TIME"
            },
            {
               fieldType: "CONST_STRING",
               constValue: "0"
            },
            {
               fieldType: "AUTO_OP"
            },
            {
               source: "a",
               target: "column1",
               fieldType: "MAPPING_STRING"
            },
            {
               source: "b",
               target: "column2",
               fieldType: "MAPPING_STRING",
               doubleQuote: false
            },
            {
               source: "_id",
               target: "column3",
               fieldType: "MAPPING_STRING"
            }
         ]   
      }   
   ]
}
```

上述配置下，生成的结果文件格式如下：

```lang-bash
$cat SDB_db1_1000_dbName_tableName_0000000001_384_201904291212.csv
"2019-04-10 14.52.17.551928","0","D","a4",b4,"5cad8cc8da342dfe37a40e84"
"2019-04-10 14.52.17.553750","0","I","a1",b1111,"5cac3850da342dfe37a40eee"
"2019-04-10 14.52.17.553820","0","B","a1",b1111,"5cac3850da342dfe37a40eee"
"2019-04-10 14.52.17.553820","0","A","a1",b22,"5cac3850da342dfe37a40eee"
```

>**Note:**
>
> 第一条为删除操作，删除记录{ "_id": { "$oid": "5cad8cc8da342dfe37a40e84" }, "a": "a4", "b": "b4" }
>
> 第二条为插入操作，插入记录{ "a": "a1", "b": "b1111" }
>
> 第三、四条为更新操作，更新前记录为：{ "_id": { "$oid": "5cac3850da342dfe37a40eee" }, "a": "a1", "b": "b1111" }，更新后操作为：{ "_id": { "$oid": "5cac3850da342dfe37a40eee" }, "a": "a1", "b": "b22" }

##使用示例##

* 重放单个日志文件

   ```lang-bash
   $./sdbreplay --hostname sdbserver1 --svcname 11810 --path /data/archivelog/archivelog.1
   ```

* 重放日志目录并且只重放某个集合的插入操作

   ```lang-bash
   $./sdbreplay --hostname sdbserver1 --svcname 11810 --path /data/archivelog --filter '{ "CL": [ "sample.employee" ], "OP": ["insert"] }'
   ```

* 在后台持续监控归档目录并重放归档日志文件，同时记录状态

   ```lang-bash
   $./sdbreplay --hostname sdbserver1 --svcname 11810 --path /data/archivelog --watch true --daemon true --status 1.status
   ```

* 根据配置文件重放归档目录下的文件
 
   ```lang-bash
   $./sdbreplay --type archive --path /data/20000/archivelog/ --filter '{ "CL": [ "sample.employee" ], "OP": ["insert", "update", "delete"], "MinLSN":468 }' --outputconf ./replay/output.conf  --status ./replay/1.status --daemon true --watch true --intervalnum 10000
   ```

##错误##

在重放过程中重放工具可能会异常退出，错误信息记录在日志文件 `sdbreplay.log` 中，可参考[错误码](reference/Sequoiadb_error_code.md)。

常见错误如下：

|错误码|原因|解决办法|
|---|---|---|
|-1|文件操作失败|检查文件是否存在|
|-3|没有文件或目录的权限|检查是否有权限操作文件或目录|
|-7|命令选项参数错误|检查修改写错的参数|
|-8|执行过程中被中断|通过 kill -15 停止了进程，指定了 status 的可以通过状态文件继续执行|
|-10|系统内部错误|需要结合日志分析|
|-98|1. 归档文件的日志不连续，无法继续重放<br/>2. 日志文件损坏|需要结合日志分析|