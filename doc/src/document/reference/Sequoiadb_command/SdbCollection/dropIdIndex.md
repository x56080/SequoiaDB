##名称##

dropIdIndex - 删除集合中的 $id 索引

##语法##

**db.collectionspace.collection.dropIdIndex\(\)**

##类别##

SdbCollection

##描述##

该函数用于删除集合中的 $id 索引，同时禁止更新或删除操作。

##参数##

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛出异常并输出错误信息。


##错误##

`dropIdIndex()` 函数常见异常如下：

|错误码  | 错误码类型 | 可能发生的原因    |  解决办法 |
| ------ | ---------- |------------- |  -------- |
| -47    | SDB_IXM_NOTEXIST |$id 索引不存在 |  -     |

当异常抛出时，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误信息码。更多错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)

##版本##

v2.0 及以上版本

##示例##

删除集合中的 $id 索引

```lang-javascript
> db.sample.employee.dropIdIndex()
```