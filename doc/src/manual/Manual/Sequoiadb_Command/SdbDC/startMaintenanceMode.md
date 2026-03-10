##名称##

startMaintenanceMode - 在复制组中开启 Maintenance 模式

##语法##

**SdbDC.startMaintenanceMode(\<options\>)**

##类别##

SdbDC 

##描述##

该函数用于在集群中对指定复制组的节点开启 Maintenance 模式，处于该模式的节点不参与复制组选举与同步一致性 replSize 计算。

> **Note:**
>
> 不能对复制组所有节点设置 Maintenance 模式。

##参数##

options（ *object，必填* ）

通过参数 options 可以指定 Maintenance 模式的参数：

- HostName（ *string* ）：Maintenance 模式生效的主机名（与 Location 互斥）

    指定的主机名需存在于当前复制组中，该主机所有节点都会开启 Maintenance 模式。

    格式：`HostName: "sdbserver"`

    > **Note:**  
    >
    > 目录复制组的主节点（Catalog 主节点）不能开启 Maintenance 模式，除非使用 Enforced 参数。

- Location（ *string* ）：Maintenance 模式生效的位置集 (与 HostName 互斥)

    指定的位置集需存在于当前集群中，该位置集所有复制组中的节点都会开启 Maintenance 模式。

    格式：`Location: "GuangZhou"`

- MinKeepTime（ *number，必填* ）：Maintenance 模式的最低运行窗口时间，取值范围为 [1, 10080]，单位为分钟

    格式：`MinKeepTime: 100`

- MaxKeepTime（ *number，必填* ）：Maintenance 模式的最高运行窗口时间，取值范围为 [1, 10080]，单位为分钟

    - MaxKeepTime 必须大于或等于 MinKeepTime
    - 系统会在 MaxKeepTime 时间到达后自动停止 Maintenance 模式

    格式：`MaxKeepTime: 200`

- Enforced（ *boolean* ）：是否强制执行 Maintenance 模式，默认为 false

    - 当设置为 true 时，允许对编目复制组的主节点开启 Maintenance 模式
    - 强制模式下需要特别注意对系统可用性的影响

    格式：`Enforced: true`

- Domain（ *string | string array* ）：按域过滤，仅对属于指定域的数据组执行操作。可以指定单个域名或域名数组

    格式：`Domain: "mydomain"` 或 `Domain: ["domain1", "domain2"]`

> **Note:**
>
> 相应复制组当前存在主节点时，不会发生切换。
> 关于时间参数的说明：
> - MinKeepTime 和 MaxKeepTime 都是必填参数
> - MinKeepTime 的取值应小于或等于 MaxKeepTime
> - 成功开启 Maintenance 模式后，在未达到 MinKeepTime 指定的时间前，节点会一直保持 Maintenance 模式
> - 在 MinKeepTime-MaxKeepTime 时间段内，如果节点状态正常，会自动解除 Maintenance 模式
> - 超过 MaxKeepTime 指定的时间后，节点将强制解除 Maintenance 模式

##返回值##

函数执行成功时，返回一个 BSON 对象，包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| MatchedNum | number | 匹配的复制组数量 |
| SucceedNum | number | 成功开启 Maintenance 模式的复制组数量 |
| IgnoredNum | number | 被忽略的复制组数量 |
| FailedNum | number | 开启失败的复制组数量 |
| FailedGroups | array | 开启失败的复制组名称列表（当 FailedNum > 0 时返回） |

函数执行失败时，将抛异常并输出错误信息。

##错误##

`startMaintenanceMode()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误或参数值超出范围 | 检查参数类型和取值范围是否正确 |
| -259 | SDB_OUT_OF_BOUND | 未指定必填参数 MinKeepTime 或 MaxKeepTime | 检查是否缺失必填参数 |
| -334 | SDB_OPERATION_CONFLICT | 复制组已处于 Critical 模式或主节点不允许开启 Maintenance 模式 | 检查复制组状态，或使用 Enforced 参数强制执行 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.8.6 及以上版本

##示例##

**示例1：** 为指定位置开启 Maintenance 模式

```lang-javascript
> var dc = db.getDC()
> dc.startMaintenanceMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000})
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例2：** 为指定主机开启 Maintenance 模式

```lang-javascript
> var dc = db.getDC()
> dc.startMaintenanceMode({HostName: "sdbserver", MinKeepTime: 60, MaxKeepTime: 300})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例3：** 强制为编目复制组主节点开启 Maintenance 模式

```lang-javascript
> var dc = db.getDC()
> dc.startMaintenanceMode({HostName: "sdbserver", MinKeepTime: 30, MaxKeepTime: 120, Enforced: true})
{
  "MatchedNum": 1,
  "SucceedNum": 1,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例4：** 仅对指定域的数据组开启 Maintenance 模式

```lang-javascript
> var dc = db.getDC()
> dc.startMaintenanceMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000, Domain: "mydomain"})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例5：** 开启 Maintenance 模式时部分复制组失败的情况

```lang-javascript
> var dc = db.getDC()
> dc.startMaintenanceMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000})
{
  "MatchedNum": 4,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 2,
  "FailedGroups": ["group3", "group4"]
}
```

**示例6：** 查看开启的 Maintenance 模式

```lang-javascript
> db.list(SDB_LIST_GROUPMODES )
{
  "GroupID": 1001,
  "GroupName": "group1",
  "GroupMode": "maintenance",
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
  "GroupMode": "maintenance",
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
