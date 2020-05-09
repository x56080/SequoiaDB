##语法##
***db.collectionspace.collection.dropIdIndex\(\)***

删除集合中的 $id 索引，同时禁止更新、删除操作。

##参数描述##

无

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误信息码。

##错误##

|错误码  | 可能的原因    |  解决方法 |
| ------ | ------------- |  -------- |
| -47    | $id索引不存在 |  -        |

##示例##

* 删除集合中的 $id 索引

```lang-javascript
> db.sample.employee.dropIdIndex()
```