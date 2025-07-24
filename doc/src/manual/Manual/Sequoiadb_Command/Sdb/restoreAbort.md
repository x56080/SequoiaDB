[^_^]:
   restoreAbort()

##名称##

restoreAbort - 中止恢复模式

##语法##

**db.restoreAbort()**

##类别##

Sdb

##描述##

集群将在执行 [restorePrepare()][restorePrepare] 或恢复重启后被设置为恢复模式，用户可通过该函数中止集群的恢复模式。

##参数##

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.0.2 及以上版本

##示例##

中止集群的恢复模式

```lang-javascript
> db.restoreAbort()
```



[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md

[restorePrepare]:manual/Manual/Sequoiadb_Command/Sdb/restorePrepare.md

