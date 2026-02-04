##名称##

startCriticalMode - 在集群中开启 Critical 模式

##语法##

**SdbDC.startCriticalMode(\<options\>)**

##类别##

SdbDC

##描述##

该函数用于集群容灾，对不具备选举条件的复制组或位置集开启 Critical 模式，并在指定的节点范围内选举出主节点。


##参数##

options（ *object，必填* ）

通过参数 options 可以指定 Critical 模式的参数：

- HostName（ *string* ）：Critical 模式生效的主机名（与 Location 互斥）

    指定的主机名需存在于当前集群中，该主机上每个复制组中的一个节点会开启 Critical 模式。

    格式：`HostName: "sdbserver"`

- Location（ *string* ）：Critical 模式生效的位置集（与 HostName 互斥）

    指定的位置集需存在于当前集群中，该位置集所有复制组中的节点都会开启 Critical 模式。

    格式：`Location: "GuangZhou"`

    > **Note:**  
    >
    > 在编目复制组开启 Critical 模式时，生效节点必须包含当前复制组的主节点。

- MinKeepTime（ *number，必填* ）：Critical 模式的最低运行窗口时间，取值范围为 [1, 10080]，单位为分钟

    格式：`MinKeepTime: 100`

- MaxKeepTime（ *number，必填* ）：Critical 模式的最高运行窗口时间，取值范围为 [1, 10080]，单位为分钟

    - MaxKeepTime 必须大于或等于 MinKeepTime
    - 系统会在 MaxKeepTime 时间到达后自动停止 Critical 模式

    格式：`MaxKeepTime: 200`

- Enforced（ *boolean* ）：是否强制开启 Critical 模式，默认值为 false

    该参数选填，取值如下：

    - true：复制组将在 Critical 模式的生效节点范围内强制生成主节点。如果在生效节点范围外存在 LSN 更高的节点，强制执行会导致数据回滚。
    - false：主节点不强制在 Critical 模式的生效节点范围内，当前LSN更高的激活节点仍然会当选主节点。

    格式：`Enforced: true`

> **Note:**
>
> 相应复制组当前存在主节点时，不会发生切换。
> 关于时间参数的说明：
> - MinKeepTime 和 MaxKeepTime 都是必填参数
> - MinKeepTime 的取值应小于或等于 MaxKeepTime
> - 成功开启 Critical 模式后，在未达到 MinKeepTime 指定的时间前，复制组会一直保持 Critical 模式
> - 在 MinKeepTime-MaxKeepTime 时间段内，如果复制组内大多数节点正常，会自动解除 Critical 模式
> - 超过 MaxKeepTime 指定的时间后，复制组将强制解除 Critical 模式

##返回值##

函数执行成功时，返回一个 BSON 对象，包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| MatchedNum | number | 匹配的复制组数量 |
| SucceedNum | number | 成功开启 Critical 模式的复制组数量 |
| IgnoredNum | number | 被忽略的复制组数量 |
| FailedNum | number | 开启失败的复制组数量 |
| FailedGroups | array | 开启失败的复制组名称列表（当 FailedNum > 0 时返回） |

函数执行失败时，将抛异常并输出错误信息。

##错误##

`startCriticalMode()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误或参数值超出范围 | 检查参数类型和取值范围是否正确 |
| -259 | SDB_OUT_OF_BOUND | 未指定必填参数 MinKeepTime 或 MaxKeepTime | 检查是否缺失必填参数 |
| -334 | SDB_OPERATION_CONFLICT | 复制组已处于 Maintenance 模式或参数范围错误 | 检查复制组状态，或检查编目的主节点是否在 Critical 模式的生效节点范围内 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.8.6 及以上版本

##示例##

**示例1：** 为指定位置开启 Critical 模式

```lang-javascript
> var dc = db.getDC()
> dc.startCriticalMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000})
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例2：** 为指定主机开启 Critical 模式

```lang-javascript
> var dc = db.getDC()
> dc.startCriticalMode({HostName: "sdbserver", MinKeepTime: 60, MaxKeepTime: 300})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例3：** 强制开启 Critical 模式

```lang-javascript
> var dc = db.getDC()
> dc.startCriticalMode({HostName: "sdbserver", MinKeepTime: 30, MaxKeepTime: 120, Enforced: true})
{
  "MatchedNum": 1,
  "SucceedNum": 1,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例4：** 开启 Critical 模式时部分复制组失败的情况

```lang-javascript
> var dc = db.getDC()
> dc.startCriticalMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000})
{
  "MatchedNum": 4,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 2,
  "FailedGroups": ["group3", "group4"]
}
```

**示例5：** 查看开启的 Critical 模式

```lang-javascript
> db.list(SDB_LIST_GROUPMODES )
{
  "GroupID": 1001,
  "GroupName": "group1",
  "GroupMode": "critical",
  "Properties": [
    {
      "NodeID": 1002,
      "NodeName": "sdbserver:11820",
      "MinKeepTime": "2024-02-03-14.30.00",
      "MaxKeepTime": "2024-02-03-19.30.00",
      "UpdateTime": "2024-02-03-14.30.00"
    }
  ]
}
{
  "GroupID": 1002,
  "GroupName": "group2",
  "GroupMode": "critical",
  "Properties": [
    {
      "NodeID": 1003,
      "NodeName": "sdbserver:11830",
      "MinKeepTime": "2024-02-03-14.30.00",
      "MaxKeepTime": "2024-02-03-19.30.00",
      "UpdateTime": "2024-02-03-14.30.00"
    }
  ]
}
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
