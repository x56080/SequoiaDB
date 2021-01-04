
##语法##
***db.collectionspace.collection.deleteLob\(\<oid\>\)***

删除集合中的大对象。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| oid    | string | 大对象的唯一描述符。 | 是 |

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误信息码。

##错误##

[错误码](manual/Manual/Sequoiadb_error_code.md)

## 示例##

* 删除一个描述符为 5435e7b69487faa663000897 的大对象

 ```lang-javascript
 > db.sample.employee.deleteLob('5435e7b69487faa663000897')
 ```
