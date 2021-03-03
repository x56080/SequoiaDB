##名称##

removeCataRG - 删除编目分区组

##语法##

***db.removeCataRG()***

##类别##

Sdb

##描述##

该函数用于删除编目分区组。该操作会删除分区组中所有编目节点，因此目标分区组中不能存在数据节点及协调节点的信息。

##参数##

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。更多错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##版本##

v1.10及以上版本

##示例##

删除编目分区组

```lang-javascript
> db.removeCataRG()
```
