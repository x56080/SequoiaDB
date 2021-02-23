##名称##

removeSpareRG - 删除数据库中的SYSSpare组

##语法##

**db.removeSpareRG()**

##类别##

Sdb

##描述##

该函数用于删除数据库中的 SYSSpare 组。

##参数##

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v3.0 及以上版本

##示例##

* 删除协调复制组

	```lang-javascript
	> db.removeSpareRG()
	```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/faq.md