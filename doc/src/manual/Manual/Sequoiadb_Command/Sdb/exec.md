
##语法##
***db.exec( \<select sql\> )***

执行 SQL 的 select 语句。

##返回值##

返回查询结果，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##示例##

* 从集合 sample.employee 中查找所有 age = 20 的记录

 ```lang-javascript
 > db.exec( "select * from sample.employee where age = 20" )
 ```