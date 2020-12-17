##语法##

***query.size()***

##类别##

SdbQuery

##描述##

返回当前游标到最终游标的记录条数。

> **Note:**
  
> query.size()返回的结果考虑 query.skip() 及 query.limit() 的影响。

##参数##

无

##返回值##

返回当前游标到最终游标的记录条数。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 选择集合 bar 下 age 大于 20 的记录（如使用 [$gt](reference/operator/match_operator/gt.md) 查询），返回当前游标到最终游标的记录条数。

  ```lang-javascript
  > db.foo.bar.find( { age: { $gt: 20 } } ).size()
  1
  ```