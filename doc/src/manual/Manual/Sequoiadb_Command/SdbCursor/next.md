
##语法##
***cursor.next()***

获取当前游标指向的下一条记录，更多查看 [cursor.current()](manual/Manual/Sequoiadb_Command/SdbCursor/current.md) 方法。

##参数描述##

无

##返回值##

返回当前游标指向的下一条记录，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。


##错误##

| 错误码 		| 可能的原因 	| 解决方法					|
| ------ 		| ------ 		| ------					|
| -31			| 上下文已关闭| 确认查询记录是否为0条	|

[错误码](manual/Manual/Sequoiadb_error_code.md)

##示例##

* 选择集合 employee 下 age 大于8的记录，返回当前游标指向的下一条记录

 ```lang-javascript
> db.sample.employee.find( { age: { $gt: 8 } } ).next()
{
      "_id": {
      "$oid": "581192bd6db4da2a23000009"
      },
      "a": 9
}
 ```
