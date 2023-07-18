##名称##

getChangeStreamToken - 获取当前节点变更流的最新位置信息

##语法##

**Sdb.getChangeStreamToken()**

##类别##

Sdb

##描述##

该函数用于获取当前节点变更流的最新位置信息。

##参数##

无

##返回值##

函数执行成功时，将返回一个 StreamToken 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v7.2.2 及以上版本

##示例##

```lang-javascript
// 获取节点变更流的最新位置信息
var token = db.getChangeStreamToken();
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[config]:manual/Manual/Database_Configuration/configuration_parameters.md