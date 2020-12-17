##名称##

removeCataRG - 删除编目分区组。 

##语法##

***db.removeCataRG()***

##类别##

Sdb

##描述##

删除编目分区组,要求编目分区组上已经没有数据节点及协调节点的信息，删除编目分区组将会把该组中所有的编目节点都删除。

##参数##

无

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##错误##

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##版本##

v1.10及以上版本。

##示例##

* 删除编目分区组

	```lang-javascript
	> db.removeCataRG()
	```
