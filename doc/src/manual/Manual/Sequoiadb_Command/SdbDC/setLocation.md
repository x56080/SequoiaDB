##名称##

setLocation - 修改集群中节点的位置信息

##语法##

**SdbDC.setLocation(<hostName>, <location>)**

##类别##

SdbDC

##描述##

该函数用于修改集群中指定主机上节点的位置信息。

##参数##

hostName（ *string，必填* ）

要修改位置信息的主机名。指定的主机名需存在于当前集群中，该主机上所有节点的位置信息都会被修改。

location（ *string，必填* ）

新的位置信息。

- 位置信息的最大长度限制为 256 字节
- 取值为空字符串时，表示删除指定主机上节点的位置信息

##返回值##

函数执行成功时，返回一个 BSON 对象，包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| MatchedNum | number | 匹配的复制组数量 |
| SucceedNum | number | 成功修改位置信息的复制组数量 |
| IgnoredNum | number | 被忽略的复制组数量 |
| FailedNum | number | 修改失败的复制组数量 |
| FailedGroups | array | 修改失败的复制组名称列表（当 FailedNum > 0 时返回） |

函数执行失败时，将抛异常并输出错误信息。

##错误##

`setLocation()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| -------- | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误或位置名称长度超过256字节 | 检查参数类型和位置名称长度是否正确 |
| -259 | SDB_OUT_OF_BOUND | 未指定必填参数 HostName 或 Location | 检查是否缺失必填参数 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

**示例1：** 将指定主机上节点的位置信息修改为"GuangZhou"

```lang-javascript
> var dc = db.getDC()
> dc.setLocation("sdbserver", "GuangZhou")
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例2：** 删除指定主机上节点的位置信息

```lang-javascript
> var dc = db.getDC()
> dc.setLocation("sdbserver", "")
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例3：** 修改位置信息时部分复制组失败的情况

```lang-javascript
> var dc = db.getDC()
> dc.setLocation("sdbserver", "Beijing")
{
  "MatchedNum": 3,
  "SucceedNum": 1,
  "IgnoredNum": 0,
  "FailedNum": 2,
  "FailedGroups": ["group1", "group2"]
}
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md