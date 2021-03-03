##语法##
***oma.close()***

关闭 oma 连接对象

##参数描述##

无

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
	
[错误码](reference/Sequoiadb_error_code.md)

> **Note:**
> 
> * oma对象为连接到目标（本地/远端机器）集群控制器（sdbcm）获得的连接对象。
> * 关闭的 oma 连接对象必须存在，否则出现异常。

##示例##

* 连接到本地的集群管理服务进程sdbcm，关闭 oma

 ```lang-javascript
> var oma = new Oma( "localhost", 11790 )
> oma.close()
 ```