##语法##
***oma.createCoord( \<svcname\>, \<dbpath\>, [config obj] )***

在目标集群控制器（sdbcm）所在的机器中创建一个 coord 节点。

##参数描述##

| 参数名     | 参数类型 | 描述         | 是否必填 |
| ---------- | -------- | ------------ | -------- |
| svcname    | string   | 节点端口号。 | 是       |
| dbpath     | string   | 节点路径。   | 是       |
| config obj | Json 对象| 节点配置信息，如配置日志大小，是否打开事务等，具体可参考[数据库配置](database_management/runtime_configuration.md)。 | 否 |

> **Note:**
>
> * oma对象为连接到目标（本地/远端机器）集群控制器（sdbcm）获得的连接对象。

##返回值##

返回节点对象，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
| 错误码 		| 可能的原因 	| 解决方法					|
| ------ 		| ------ 		| ------					|
| -6			| 参数错误      | 确认参数类型和参数个数是否正确	|
| -145			| 节点已存在    | 使用列表查看节点是否存在	|
| -157			| 节点配置冲突  | 使用列表查看节点dbpath等是否冲突	|
	
[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 在本地创建一个端口号为11810的 coord 节点，将该节点关联到指定的 catalog 节点

 ```lang-javascript
> var oma = new Oma( "localhost", 11790 )
> oma.createCoord( 11810, "/opt/sequoiadb/database/coord/11810", { catalogaddr: "ubuntu1:11823, ubuntu2:11823" } )
 ```
