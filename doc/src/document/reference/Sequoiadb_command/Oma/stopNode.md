##语法##
***oma.stopNode( \<svcname\> )***

在目标集群控制器（sdbcm）所在的机器中停止一个节点。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| svcname | string | 节点端口号。 | 是 |

> **Note:**
> 
> * oma对象为连接到目标（本地/远端机器）集群控制器（sdbcm）获得的连接对象。
> * 指定停止的节点必须存在，否则出现异常。

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
| 错误码 		| 可能的原因 	| 解决方法					|
| ------ 		| ------ 		| ------					|
| -6			| 参数错误    | 查看参数是否填写正确	|

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 在本地停止一个端口号为11830的节点

 ```lang-javascript
> var oma = new Oma( "localhost", 11790 )
> oma.stopNode( 11830 )
 ```
