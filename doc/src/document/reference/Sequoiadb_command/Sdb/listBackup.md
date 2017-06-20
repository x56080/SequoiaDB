##语法##
***db.listBackup( [options], [cond], [sel], [sort] )***

枚举数据库备份。

##参数描述##

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
| GroupName | 指定备份的复制组名，缺省为所有复制组 	| GroupName:"data1" 或 GroupName:["data1", "data2"] 																					|
| Name 		| 指定备份名称，缺省为所有备份 			| Name:"backup-2014-1-1" 				|
| Path 		| 指定备份路径，缺省为配置参数指定的备份路径。该路径支持通配符（%g/%G: group name, %h/%H: host name, %s/%S:service name） 														| Path:"/opt/sequoiadb/backup/%g" |
| IsSubDir 	| 上述 Path 参数所配置的路径是否为配置参数指定的备份路径的子目录，缺省为 false IsSubDir:false 																				| Prefix 备份前缀名，支持通配符（%g,%G,%h,%H,%s,%S），缺省为空 Prefix:"%g_bk_" 				|
| Detail    | 是否显示详细信息，缺省为 false        | Detail: true |

##返回值##
返回游标对象，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

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
