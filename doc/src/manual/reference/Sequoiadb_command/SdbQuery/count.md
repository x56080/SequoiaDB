##语法##

***query.count()***

##类别##

SdbQuery

##描述##

查询符合匹配条件的记录条数。

> **Note:** 
 
> query.count() 返回的结果忽略 [query.skip()](reference/Sequoiadb_command/SdbQuery/skip.md) 及 [query.limit()](reference/Sequoiadb_command/SdbQuery/limit.md) 的影响。

##参数##

无

##返回值##

返回符合匹配条件的记录条数。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 选择集合 bar 下 age 大于10的记录，使用 [$gt](reference/operator/match_operator/gt.md) 查询返回符合匹配条件 { age: { $gt: 10 } } 的记录条数。

  ```lang-javascript
  > db.foo.bar.find( { age: { $gt: 10 } } ).count()
  3
  ```
