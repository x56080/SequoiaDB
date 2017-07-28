##语法##
***db.startRG( \<name\> )***

启动指定的分区组。分区组启动后才能在分区组上创建节点。这个方法等价于[rg.start()](reference/Sequoiadb_command/SdbReplicaGroup/start.md)。

##参数描述##

| 参数名 | 参数类型 | 描述 			| 是否必填 	 |
| ------ | ------ 	| ------ 		| ------	 |
| name 	 | string 	| 分区组的名称 	| 是 		 |

**Note:**

> * 若指定的分区组不存在，将抛异常。

##返回值##
无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

## 示例##

* 启动分区组的命令如下：

	```lang-javascript
	> db.startRG( "group1" )
	> db.startRG( "group2", "group3", "group4" )
	```