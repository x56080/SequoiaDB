归档的日志可以通过日志重放工具sdbreplay在其它集群或节点重新执行。

##基本功能##

日志重放工具主要功能：

1. 读取日志并在SequoiaDB上执行；
2. 可以根据条件过滤日志（日志文件、集合空间、集合、操作、LSN）；
3. 支持持续监控归档目录并重放日志；
4. 支持在后台执行；
5. 可以生成状态文件从而在退出重启后接续执行退出前的重放。

目前重放工具支持重放的日志操作有：

|操作|说明|
|---|---|
|insert|插入数据|
|update|更新数据|
|delete|删除数据|
|truncatecl|truncate集合|

> Note:  
> 1. 重放时根据归档文件的FileId从小到大重放；  
> 2. 重放出错时重放工具会立即退出，已经重放的日志不会回滚；  
> 3. 归档日志LSN不连续时重放工具会报错退出；  
> 4. 通过status选项指定状态文件，可以从上次退出的地方继续重放，不指定时每次从第一个归档文件开始重放；  
> 5. 未重放的归档日志文件发生了移动操作后，移动文件不会被重放；已重放的归档日志文件发生了移动操作，重放工具会回滚移动文件中的日志操作；  
> 6. 不支持回滚truncatecl操作，回滚时遇到truncatecl操作会报错退出；  
> 7. 在后台执行时可以通过kill -15 <pid>的方式停止进程。  
> 8. 复制日志是幂等的，同一条日志多次重放结果不变。  
> 9. 归档重放过程中，如果在集合上执行split操作，通过协调节点重放到同一个集群时可能会丢失数据。因为split的源复制组在数据迁移到目标复制组后会删除本地相应数据，并生成删除日志；而目标复制组接收数据后生成插入日志。如果目标复制组的归档日志先被重放而后源复制组的归档日志才被重放，那么迁移的那部分数据重放时先插入后被删除，导致数据丢失。此时可以通过再次重放目标复制组的归档日志来重新插入丢失的数据。

##命令选项##

|选项|缩写|描述|类型|默认值|说明|
|---|---|---|---|---|---|
|help|h|打印帮助信息|-|-|
|version|V|打印版本号|-|-|
|hostname|-|SequoiaDB所在的主机名|string|-|dump和dumpheader为false时必填|
|svcname|-|SequoiaDB的服务名（端口号）|string|-|dump和dumpheader为false时必填|
|user|-|用户名|string|-|
|password|-|密码|string|-|
|path|-|归档目录|string|-|必填，可以是文件或目录|
|filter|-|过滤条件|string(json)|-|
|dump|-|只导出日志，不重放|bool|false|
|dumpheader|-|只导出归档文件头，不重放|bool|false|
|delete|-|重放后删除完成重放的归档日志文件|bool|false|只删除完整归档文件|
|watch|-|持续监控归档目录并重放日志|bool|false|path为目录时有效|
|daemon|-|在后台运行|bool|false|kill -15 \<pid>可以使后台进程正确退出|
|status|-|指定状态文件|string|-|状态文件会存储重放的状态信息，首次指定时重放工具会生成该文件。重放工具退出后，通过指定状态文件可以从上次退出的地方继续重放。|

其中filter是json格式的字符串，可以指定过滤条件对日志进行过滤，过滤条件有：

|字段|类型|说明|
|---|---|---|
|OP|array[string]|指定重放的操作，默认为重放工具支持的所有日志操作|
|ExclOP|array[string]|指定排除的操作，优先级高于OP|
|MinLSN|int64|指定重放的最小LSN（包括该LSN）|
|MaxLSN|int64|指定重放的最大LSN（不包括该LSN）|
|File|array[string]|指定重放的日志文件，默认为全部|
|ExclFile|array[string]|指定排除的日志文件，优先级高于File|
|CS|array[string]|指定重放的集合空间，默认为全部|
|ExclCS|array[string]|指定排除的集合空间，优先级高于CS|
|CL|array[string]|指定重放的集合，集合名的格式为"集合空间.集合名"，默认为全部|
|ExclCL|array[string]|指定排除的集合，优先级高于CL|

> Note:  
> 1. 命令选项watch在path指定为目录时有效；  
> 2. dump或dumpheader为true不需要设置hostname和svcname。

##使用示例##

1. 重放日志文件

  ```
  $./sdbreplay --hostname sdbserver1 --svcname 11810 --path /data/archivelog/archivelog.1
  ```

2. 重放日志目录并且只重放某个集合的插入操作

  ```
  $./sdbreplay --hostname sdbserver1 --svcname 11810 --path /data/archivelog --filter '{ "CL": [ "foo.bar" ], "OP": ["insert"] }'
  ```

3. 在后台持续监控归档目录并重放归档日志文件，同时记录状态

  ```
  $./sdbreplay --hostname sdbserver1 --svcname 11810 --path /data/archivelog --watch true --daemon true --status 1.status
  ```

##错误##

在重放过程中重放工具可能会异常退出，错误信息记录在日志文件sdbreplay.log中，可参考[错误码](reference/Sequoiadb_error_code.md)。

常见错误有：

|错误码|原因|解决办法|
|---|---|---|
|-1|文件操作失败|检查文件是否存在|
|-3|没有文件或目录的权限|检查是否有权限操作文件或目录|
|-7|命令选项参数错误|检查修改写错的参数|
|-8|执行过程中被中断|通过kill -15停止了进程，指定了status的可以通过状态文件继续执行|
|-10|系统内部错误|需要结合日志分析|
|-98|1. 归档文件的日志不连续，无法继续重放<br/>2. 日志文件损坏|需要结合日志分析|