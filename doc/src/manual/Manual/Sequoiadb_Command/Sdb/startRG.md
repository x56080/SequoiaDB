
##语法##
***db.startRG( \<name1\>，[name2]，...)***

启动指定的复制组。复制组启动后才能在复制组上创建节点。这个方法等价于[rg.start()](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/start.md)。

##参数描述##

| 参数名 | 参数类型 | 描述 			| 是否必填 	 |
| ------ | ------ 	| ------ 		| ------	 |
| name1，name2... 	 | string 	| 复制组的名称 	| 是 		 |

>**Note:**
>
> 若指定的复制组不存在，将抛异常。

##返回值##
无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。

关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##错误##
常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

## 示例##

* 启动复制组的命令如下：

	```lang-javascript
	> db.startRG( "group1" )
	> db.startRG( "group2", "group3", "group4" )
	```