##语法##
***oma.removeCoord( \<svcname\> )***

在集群中删除指定的 coord 节点。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| svcname | string | 节点端口号。 | 是 |

> **Note:**
> 
> * oma对象为连接到目标（本地/远端机器）集群控制器（sdbcm）获得的连接对象。
> * 指定删除的节点必须存在，否则出现异常。

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
| 错误码 		| 可能的原因 	| 解决方法					|
| ------ 		| ------ 		| ------					|
| -3			| 权限错误      | 确认oma是否有权限操作该节点	|
| -146			| 节点不存在    | 使用列表查看节点是否存在	|

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 在集群的ubuntu1机器上删除一个端口号为11810的 coord 节点

 ```lang-javascript
> var oma = new Oma( "ubuntu1", 11790 )
> oma.removeCoord( 11810 )
 ```
