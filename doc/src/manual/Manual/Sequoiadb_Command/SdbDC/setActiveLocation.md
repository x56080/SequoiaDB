##名称##

setActiveLocation - 设置集群的 ActiveLocation

##语法##

**SdbDC.setActiveLocation(\<location\>, [options])**

##类别##

SdbDC

##描述##

该函数用于在集群中设置指定的位置集为 ActiveLocation。

##参数##

location（ *string，必填* ）

位置集名称

- 指定的位置集需存在于当前集群中。
- 取值为空字符串时，表示删除当前集群的 ActiveLocation。

options（ *object，选填* ）

通过参数 options 可以指定过滤条件：

- Domain（ *string | string array* ）：按域过滤，仅对属于指定域的数据组执行操作。可以指定单个域名或域名数组

    格式：`Domain: "mydomain"` 或 `Domain: ["domain1", "domain2"]`

##返回值##

函数执行成功时，返回一个 BSON 对象，包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| MatchedNum | number | 匹配的复制组数量 |
| SucceedNum | number | 成功设置 ActiveLocation 的复制组数量 |
| IgnoredNum | number | 被忽略的复制组数量 |
| FailedNum | number | 设置失败的复制组数量 |
| FailedGroups | array | 设置失败的复制组名称列表（当 FailedNum > 0 时返回） |

函数执行失败时，将抛异常并输出错误信息。

##错误##

`setActiveLocation()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误 | 检查参数类型是否正确 |
| -259 | SDB_OUT_OF_BOUND | 未指定必填参数 | 检查是否缺失必填参数 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

**示例1：** 将位置集"GuangZhou"设置为集群的 ActiveLocation

```lang-javascript
> var dc = db.getDC()
> dc.setActiveLocation("GuangZhou")
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例2：** 删除集群的 ActiveLocation

```lang-javascript
> var dc = db.getDC()
> dc.setActiveLocation("")
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例3：** 仅对指定域的数据组设置 ActiveLocation

```lang-javascript
> var dc = db.getDC()
> dc.setActiveLocation("GuangZhou", {Domain: "mydomain"})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例4：** 设置 ActiveLocation 时部分复制组失败的情况

```lang-javascript
> var dc = db.getDC()
> dc.setActiveLocation("Beijing")
{
  "MatchedNum": 4,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 2,
  "FailedGroups": ["group3", "group4"]
}
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md