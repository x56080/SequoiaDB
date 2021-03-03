##语法##
***oma.removeOM( \<svcname\> )***

删除指定的 sdbom 服务进程。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| svcname | string | 节点端口号。 | 是 |

> **Note:**
> 
> * oma对象为连接到目标（本地/远端机器）集群控制器（sdbcm）获得的连接对象。

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。


##错误##
| 错误码 		| 可能的原因 	| 解决方法					|
| ------ 		| ------ 		| ------					|
| -146			| 节点不存在    | 查看节点是否存在	|

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 删除安装在本地的 sdbom 服务进程

 ```lang-javascript
> var oma = new Oma( "localhost", 11790 )
> oma.removeOM( 11830 )
 ```