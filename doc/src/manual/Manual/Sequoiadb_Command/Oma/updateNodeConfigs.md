
##名称##

updateNodeConfigs - 对指定端口的数据库节点，用新的节点配置信息更新该节点原来配置文件上的配置信息。

##语法##

**oma.updateNodeConfigs(\<svcname\>,\<config\>)**

##类别##

Oma

##描述##

对指定端口的数据库节点，用新的节点配置信息更新该节点原来配置文件上的配置信息。

**Note:**

* 更新的配置信息需要重启节点生效。

##参数##

* `svcname` ( *Int | String*， *必填* )

	节点端口号。

* `config` ( *Object*， *必填* )

	节点配置信息，如配置日志大小，是否打开事务等，具体可参考[数据库配置](manual/Manual/Database_Configuration/configuration_parameters.md)。

##返回值##

成功：无。  

失败：抛出异常。

##错误##

`updateNodeConfigs()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -6     | SDB_INVALIDARG | 非法输入参数。| 检查端口号和配置信息是否正确。 |
| -259   | SDB_OUT_OF_BOUND | 未输入节点端口号或配置信息。| 输入节点端口号以及配置信息。 |

当异常抛出时，可以通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取[错误码](manual/Manual/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息。
可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)了解更多内容。

##示例##

1. 更新端口号为 11810 的节点的配置。

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.setNodeConfigs( 11810, { diaglevel: 3 } )
	```