##名称##

removeBackup - 删除数据库备份

##语法##

**db.removeBackup( [options] )**

##类别##

Sdb

##描述##

该函数用于删除数据库备份。

##参数##

| 参数名 	| 参数类型 	| 描述 									 | 是否必填 |
| ----------| ----------| ---------------------------------------| ------ 	|
| options 	| Json 对象 | 设定备份名、复制组、备份路径等参数 	 | 否 		|

###options 格式###

| 属性名 	| 描述 										| 格式 		|
| ------ 	| ----------------------------------------- | ------ 	|
| GroupID 	| 备份的复制组 ID，缺省为所有复制组 		| GroupID:1000 或 GroupID:[1000, 1001] |
| GroupName | 备份的复制组名，缺省为所有复制组 			| GroupName:"data1" 或 GroupName:["data1", "data2"] |
| Name 		| 备份名称，缺省删除所有备份 				| Name:"backup-2014-1-1" |
| Path 		| 备份路径，缺省为配置参数指定的备份路径。该路径支持通配符（%g/%G: group name, %h/%H: host name, %s/%S:service name）。当在协调节点上执行命令使用该参数时，需要使用通配符，以避免所有的节点往同一个路径下进行操作而导致未知IO错误。 | Path:"/opt/sequoiadb/backup/%g" |
| IsSubDir 	| 上述 Path 参数所配置的路径是否为配置参数指定的备份路径的子目录，如果为true，则真实的备份目录为：" 配置参数中指定的备份目录 / `Path`目录 "；缺省为 false。 | IsSubDir:false |
| Prefix 	| 备份前缀名，支持通配符（%g,%G,%h,%H,%s,%S），缺省为空 | Prefix:"%g_bk_" |
| ID        | 备份 ID，-1表示该名字的所有备份, 缺少为 -1 | ID: -1 |

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.8.2 及以上版本增加 `ID` 参数。

##示例##

* 删除数据库中备份名为“backup-2014-1-1”的备份信息

	```lang-javascript
	> db.removeBackup({Name:"backup-2014-1-1"})
	```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md