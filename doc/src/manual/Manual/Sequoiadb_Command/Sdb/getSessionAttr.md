##语法##

***db.getSessionAttr()***

获取会话属性

##描述##

该函数用于获取会话属性。

> **Note:**
>
> 如果当前会话属性不符合预期，可使用 [Sdb.setSessionAttr()](manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md) 设置会话属性。

##参数##

无

##返回值##

函数执行成功时，将返回表示会话属性的 Json 对象，返回值字段信息可参考 [Sdb.setSessionAttr()](manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md)。

函数执行失败时，将抛异常并输出错误信息。可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码，关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##示例##

* 获取会话属性

 ```lang-javascript
 > db.getSessionAttr()
 {
   "PreferedInstance": "M",
   "PreferedInstanceMode": "random",
   "PreferedStrict": false,
   "Timeout": -1,
   "TransIsolation": 0,
   "TransTimeout": 60,
   "TransUseRBS": true,
   "TransLockWait": false,
   "TransAutoCommit": false,
   "TransAutoRollback": true,
   "TransRCCount": true,
   "Source": ""
 }
 ```
