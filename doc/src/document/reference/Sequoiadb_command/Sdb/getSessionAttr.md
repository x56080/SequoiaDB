##语法##

***db.getSessionAttr()***

获取会话属性

##描述##

该函数用于获取会话属性。

> **Note:**
>
> 如果当前会话属性不符合预期，可使用 [Sdb.setSessionAttr()](reference/Sequoiadb_command/Sdb/setSessionAttr.md) 设置会话属性。

##参数##

无

##返回值##

函数执行成功时，将返回表示会话属性的 Json 对象，返回值字段信息可参考 [Sdb.setSessionAttr()](reference/Sequoiadb_command/Sdb/setSessionAttr.md)。

函数执行失败时，将抛异常并输出错误信息。可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码，关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##示例##

* 获取会话属性

 ```lang-javascript
 > db.getSessionAttr()
 {
   "PreferedInstance": "M",
   "PreferredInstance": "M",
   "PreferedInstanceMode": "random",
   "PreferredInstanceMode": "random",
   "PreferedStrict": false,
   "PreferredStrict": false,
   "PreferedPeriod": 60,
   "PreferredPeriod": 60,
   "Timeout": -1,
   "TransIsolation": 0,
   "TransTimeout": 60,
   "TransUseRBS": true,
   "TransLockWait": false,
   "TransAutoCommit": false,
   "TransAutoRollback": true,
   "TransRCCount": true,
   "TransAllowLockEscalation": true,
   "TransMaxLockNum": 10000,
   "TransMaxLogSpaceRatio": 50,
   "Source": ""
 }
 ```

> **Note:**
>
> v3.2.9 及以上版本中，字段 PreferedInstance、PreferedInstanceMode、PreferedStrict 和 PreferedPeriod 已更名为 PreferredInstance、PreferredInstanceMode、PreferredStrict 和 PreferredPeriod，用户应使用更名后的字段。为保证兼容性，SequoiaDB 由低版本升级至 v3.2.9 及以上版本后，仍兼容原字段。

