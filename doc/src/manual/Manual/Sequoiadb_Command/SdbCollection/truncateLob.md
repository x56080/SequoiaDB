
##语法##
***db.collectionspace.collection.truncateLob\(\<oid\>, \<length\>\)***

截短集合中的大对象。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| oid    | string | 大对象的唯一描述符。 | 是 |
| length | int | 截短到的长度，必须是大于等于0的值。当length大于等于大对象的大小时，大对象不发生变化。 | 是 |

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误信息码。

##错误##

[错误码](manual/Manual/Sequoiadb_error_code.md)

## 示例##

* 截短一个描述符为'5435e7b69487faa663000897'的大对象的长度到0

 ```lang-javascript
 > db.sample.employee.truncateLob('5435e7b69487faa663000897', 0)
 ```
