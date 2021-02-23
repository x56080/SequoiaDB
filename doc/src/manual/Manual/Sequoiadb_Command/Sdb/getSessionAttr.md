##名称##

getSessionAttr - 获取会话属性

##语法##

**db.getSessionAttr()**

##类别##

Sdb

##描述##

该函数用于获取会话属性。

> **Note:**
>
> 如果当前会话属性不符合预期，可使用 [Sdb.setSessionAttr()](manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md) 设置会话属性。

##参数##

无

##返回值##

函数执行成功时，将通过 BSONObj 方式返回会话属性的详细信息列表，返回的字段信息可参考 [Sdb.setSessionAttr()](manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md)。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.8 及以上版本

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

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/faq.md