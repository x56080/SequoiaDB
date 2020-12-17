##语法##
***rg.start()***

启动当前分区组。分区组启动之后才能创建节点及其他操作。也可以使用方法 [db.startRG(< name >))](reference/Sequoiadb_command/Sdb/startRG.md) 启动指定的节点。

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过 [getLastErrMsg](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息，或通过 [getLastError](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。
关于错误处理可以参考 [常见错误处理指南](manual/faq.md) 。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

启动 group1 分区组

```lang-javascript
> var rg = db.getRG("group1")
> rg.start()   //等价于 db.startRG("group")
```
