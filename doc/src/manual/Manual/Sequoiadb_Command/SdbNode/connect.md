
##语法##
***node.connect()***

将数据库连接到指定节点。连接之后能进行一系列的操作，可以使用 node.connect().help() 查看相关的操作。

##参数描述##

无

##返回值##

返回当前节点对象，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。


##错误##
| 错误码 		| 可能的原因 	| 解决方法					|
| ------ 		| ------ 		| ------					|
| -15			| 网络错误    | 检查语法，查看节点是否启动	|

[错误码](manual/Manual/Sequoiadb_error_code.md)

##示例##

* 将数据库连接到节点名为 node 上

 ```lang-javascript
 > node.connect()
  vmsvr2-suse-x64:11800
 ```

