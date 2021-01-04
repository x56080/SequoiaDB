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

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。
关于错误处理可以参考[常见错误处理指南][faq]。

##错误##

常见错误可参考[错误码][Sequoiadb_error_code]。

##版本##

v1.10及以上版本。

##示例##

* 删除编目分区组

	```lang-javascript
	> db.removeCataRG()
	```


[^_^]:
    本文使用的所有链接及引用
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/faq.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md