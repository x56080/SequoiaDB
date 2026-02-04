##名称##

stopCriticalMode - 停止集群 Critical 模式

##语法##

**dc.stopCriticalMode([options])**

##类别##

SdbDC

##描述##

该函数用于容灾恢复。停止集群的 Critical 模式，恢复正常运行状态。

##参数##

options（ *object，可选* ）

通过参数 options 可以指定 Critical 模式的参数：

- HostName（ *string* ）：Critical 模式停止生效的主机名（与 Location 互斥）

    指定的主机名需存在于当前集群中，该主机上的复制组将停止 Critical 模式。

    格式：`HostName: "sdbserver"`

- Location（ *string* ）：Critical 模式停止生效的位置集（与 HostName 互斥）

    指定的位置集需存在于当前集群中，该位置集所有复制组都会停止 Critical 模式。

    格式：`Location: "GuangZhou"`

##返回值##

函数执行成功时，返回一个 BSON 对象，包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| MatchedNum | number | 匹配的复制组数量 |
| SucceedNum | number | 成功停止 Critical 模式的复制组数量 |
| IgnoredNum | number | 被忽略的复制组数量 |
| FailedNum | number | 停止失败的复制组数量 |
| FailedGroups | array | 停止失败的复制组名称列表（当 FailedNum > 0 时返回） |

函数执行失败时，将抛异常并输出错误信息。

##错误##

`stopCriticalMode()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误或参数值超出范围 | 检查参数类型和取值范围是否正确 |
| -334 | SDB_OPERATION_CONFLICT | 复制组处于 Maintenance 模式 | 检查复制组状态 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.8.6 及以上版本

##示例##

**示例1：** 停止集群 Critical 模式

```lang-javascript
> var dc = db.getDC()
> dc.stopCriticalMode()
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
