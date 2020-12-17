##语法##

***db.execUpdate( \<other sql\> )***

执行 SQL 除 select 以外的其它语句

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##示例##

* 向集合 foo.bar 中插入新的记录

 ```lang-javascript
 > db.execUpdate( "insert into foo.bar(name,age) values('zhangshang', 30)" )
 ```