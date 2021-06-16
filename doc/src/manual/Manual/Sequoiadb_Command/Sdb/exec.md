##名称##

exec - 执行 SQL 的 select 语句

##语法##

**db.exec( \<select sql\> )**

##类别##

Sdb

##描述##

该函数用于执行 SQL 的 select 语句。

##参数##

无

##返回值##

函数执行成功时，将通过游标（SdbCursor）方式返回查询结果信息列表。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

从集合 sample.employee 中查找所有 age = 20 的记录

```lang-javascript
> db.exec( "select * from sample.employee where age = 20" )
```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md