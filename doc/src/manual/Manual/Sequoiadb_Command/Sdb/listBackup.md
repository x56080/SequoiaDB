##名称##

listBackup - 枚举数据库备份

##语法##

**db.listBackup( [options], [cond], [sel], [sort] )**

##类别##

Sdb

##描述##

该函数用于在当前数据库中枚举数据库备份。

##参数##

| 参数名  | 参数类型  | 描述                               						| 是否必填 |
|---------| ----------| --------------------------------------------------------|----------|
| options | Json 对象 | 指定备份名、复制组、路径等参数 	   						| 否       |
| cond    | Json 对象 | 备份过滤条件                       						| 否       |
| sel     | Json 对象 | 选择备份输出的字段             	   						| 否       |
| sort    | Json 对象 | 对返回的记录按选定的字段排序。1为升序；-1为降序。       | 否 	   |


###Options格式###

| 参数名 	| 描述 									| 格式 									|
|-----------|---------------------------------------|---------------------------------------|
| GroupID 	| 指定备份的复制组 ID，缺省为所有复制组 | GroupID:1000 或 GroupID:[1000, 1001]  |
| GroupName | 指定备份的复制组名，缺省为所有复制组 	| GroupName:"data1" 或 GroupName:["data1", "data2"] |
| Name 		| 指定备份名称，缺省为所有备份 			| Name:"backup-2014-1-1" 				|
| Path 		| 指定备份路径，缺省为配置参数指定的备份路径。该路径支持通配符（%g/%G: group name, %h/%H: host name, %s/%S:service name）。当在协调节点上执行命令使用该参数时，需要使用通配符，以避免所有的节点往同一个路径下进行操作而导致未知IO错误。 	| Path:"/opt/sequoiadb/backup/%g" |
| IsSubDir 	| 上述 Path 参数所配置的路径是否为配置参数指定的备份路径的子目录，如果为true，则真实的备份目录为：" 配置参数中指定的备份目录 / `Path`目录 "；缺省为 false | IsSubDir:false |
| Prefix    | 备份前缀名，支持通配符（%g,%G,%h,%H,%s,%S），缺省为空 | Prefix:"%g_bk_" |
| Detail    | 是否显示详细信息，缺省为 false        | Detail: true |

##返回值##

函数执行成功时，将返回游标对象。通过游标对象获取的结果字段说明可查看 [$LIST_BACKUP][LIST_BACKUP]

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

* 查看数据库配置参数指定的备份路径下的所有备份信息

	```lang-javascript
	> db.listBackup()
	{
	  "Version": 2,
	  "Name": "test",
	  "ID": 0,
	  "NodeName": "vmsvr2-suse-x64-1:20000",
	  "GroupName": "db1",
	  "EnsureInc": false,
	  "BeginLSNOffset": 195652020,
	  "EndLSNOffset": 195652068,
	  "TransLSNOffset": -1,
	  "StartTime": "2017-06-20-13:02:22",
	  "LastLSN": 195652020,
	  "LastLSNCode": 1845751176,
	  "HasError": false
	}
	```

* 查看其它路径下的备份信息

  备份数据节点到其它路径

    ```lang-javascript
    > var datadb = new Sdb( "localhost", 20000 )
    > datadb.backup( { Path: "/tmp/sequoiadb_backup/20000" } )
    ```

  连接 coord 查看备份信息，listBackup 时需要指定 Path 参数，否则在默认路径下查找不到备份信息

    ```lang-javascript
    > var db = new Sdb( "localhost", 11810 ) 
    > db.listBackup( { Path: "/tmp/sequoiadb_backup/20000" } )
    {
      "Version": 2,
      "Name": "2017-10-26-10:14:11",
      "ID": 0,
      "NodeName": "ubuntu-test-03:20000",
      "GroupName": "db1",
      "EnsureInc": false,
      "BeginLSNOffset": -1,
      "EndLSNOffset": 375546828,
      "TransLSNOffset": -1,
      "StartTime": "2017-10-26-10:14:11",
      "LastLSN": -1,
      "LastLSNCode": 0,
      "HasError": false
    }


[^_^]:
     本文使用的所有引用及链接
[LIST_BACKUP]:manual/Manual/SQL_Grammar/Monitoring/LIST_BACKUP.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/faq.md