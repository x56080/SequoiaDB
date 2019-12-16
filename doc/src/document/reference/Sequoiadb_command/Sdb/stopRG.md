##语法##
***db.stopRG( \<name1\>，[name2]，... )***

停止指定的分区组。停止后将不能执行创建节点等相关操作。这个方法等价于[rg.stop()](reference/Sequoiadb_command/SdbReplicaGroup/stop.md)。

##参数描述##

| 参数名             | 参数类型 | 描述 			| 是否必填 	 |
| ------             | ------ 	| ------ 		| ------	 |
| name1，name2... 	 | string 	| 分区组的名称 	| 是 		 |

**Note:**

> * 若指定的分区组不存在，将抛异常；若不指定任何分区组，该操作为空操作。

##返回值##
无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

## 示例##

* 停止分区组的命令如下：

	```lang-javascript
	> db.stopRG( "group1" )
	> db.stopRG( "group2", "group3", "group4" )
	```