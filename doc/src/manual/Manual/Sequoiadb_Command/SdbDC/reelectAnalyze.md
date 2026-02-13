##名称##

reelectAnalyze - 分析集群中复制组的主节点分布合理性，或将主节点切换至合理的节点上

##语法##

**SdbDC.reelectAnalyze([option], [run])**

##类别##

SdbDC

##描述##

该函数用于分析集群中复制组的主节点分布是否合理。通过查询各节点的选主权重（RunStatusWeight），判断当前主节点是否在权重最高的节点上。如果不在，则生成切主计划。当 run 为 true 时，还会执行切主操作。

##参数##

option（ *object，选填* ）

通过参数 option 可以指定过滤条件和分析选项，为 null 或不指定时分析所有数据组：

- HostName（ *string* ）：按主机名过滤，仅分析包含指定主机名节点的数据组

    格式：`HostName: "sdbserver"`

- Domain（ *string | string array* ）：按域过滤，仅分析属于指定域的数据组。可以指定单个域名或域名数组

    格式：`Domain: "mydomain"` 或 `Domain: ["domain1", "domain2"]`

- FilterLevel（ *string* ）：过滤级别，用于筛选参与权重比较的节点，取值如下，默认为 "Weight"

    - "GroupMode"：仅考虑 "Critical" 或 "Maintenance" 的节点
    - "Location"：在 GroupMode 基础上，还包含 "ActiveLocation" 和 "AffinitiveLocation" 的节点
    - "Weight"：考虑所有节点

    格式：`FilterLevel: "Location"`

- Rebalance（ *boolean* ）：是否启用主节点均衡，默认为 true

    启用后，主节点在最高权重相同的多个节点中进行均衡分配。

    格式：`Rebalance: true`

- Seconds（ *number* ）：选举的超时时间，默认值为 30，单位为秒。仅在 run 为 true 时生效

    格式：`Seconds: 60`

- Level（ *number* ）：重选举等待级别，取值：[1, 3]，默认为 3。仅在 run 为 true 时生效

    - 1：等待当前写操作结束，并阻塞后续写操作
    - 2：等待写游标（大对象操作）结束
    - 3：等待事务结束

    格式：`Level: 3`

run（ *boolean，选填* ）

指定是否执行切主操作，默认为 false：

- false：仅分析并输出切主计划，不实际执行
- true：分析后执行切主操作

> **Note:**
>
> - 当某个组没有主节点时，该组将被计入失败（FailedNum）。
> - Rebalance 仅在最高权重相同的候选节点之间进行均衡选择。
> - Seconds、Level 参数会传递给每个复制组的 [rg.reelect()][rg_reelect] 操作。

##返回值##

函数执行成功时，返回一个 BSON 对象，包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| MatchedNum | number | 匹配的复制组数量 |
| SucceedNum | number | 需要切主（或已成功切主）的复制组数量 |
| IgnoredNum | number | 被忽略的复制组数量（当前主节点已是最优或无候选节点） |
| FailedNum | number | 失败的复制组数量（无主节点或切主失败） |
| FailedGroups | array | 失败的复制组名称列表（当 FailedNum > 0 时返回） |
| Detail | array | 切主计划详细信息列表 |

Detail 数组中每个元素包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| GroupName | string | 复制组名称 |
| OldPrimary | string | 当前主节点地址，格式为 hostname:svcname |
| NewPrimary | string | 建议（或已切换）的新主节点地址，格式为 hostname:svcname |
| CausedBy | string | 导致切主的原因，取值包括 "Critical"、"ActiveLocation"、"AffinitiveLocation"、"Maintenance"、"Weight"、"Rebalance" |

函数执行失败时，将抛异常并输出错误信息。

##错误##

`reelectAnalyze()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误或 FilterLevel 取值不合法 | 检查参数类型是否正确，确认 FilterLevel 为 GroupMode、Location 或 Weight |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.8.6 及以上版本

##示例##

**示例1：** 分析所有数据组的主节点分布

```lang-javascript
> var dc = db.getDC()
> dc.reelectAnalyze()
{
  "MatchedNum": 4,
  "SucceedNum": 2,
  "IgnoredNum": 2,
  "FailedNum": 0,
  "Detail": [
    {
      "GroupName": "db1",
      "OldPrimary": "sdbserver1:20000",
      "NewPrimary": "sdbserver2:20000",
      "CausedBy": "Critical"
    },
    {
      "GroupName": "db3",
      "OldPrimary": "sdbserver1:20020",
      "NewPrimary": "sdbserver2:20020",
      "CausedBy": "Weight"
    }
  ]
}
```

**示例2：** 按 GroupMode 级别过滤分析

```lang-javascript
> var dc = db.getDC()
> dc.reelectAnalyze({FilterLevel: "GroupMode"})
{
  "MatchedNum": 4,
  "SucceedNum": 1,
  "IgnoredNum": 3,
  "FailedNum": 0,
  "Detail": [
    {
      "GroupName": "db1",
      "OldPrimary": "sdbserver1:20000",
      "NewPrimary": "sdbserver2:20000",
      "CausedBy": "Critical"
    }
  ]
}
```

**示例3：** 按主机名和域过滤后执行切主

```lang-javascript
> var dc = db.getDC()
> dc.reelectAnalyze({HostName: "sdbserver1", Domain: "mydomain"}, true)
{
  "MatchedNum": 2,
  "SucceedNum": 1,
  "IgnoredNum": 1,
  "FailedNum": 0,
  "Detail": [
    {
      "GroupName": "db1",
      "OldPrimary": "sdbserver1:20000",
      "NewPrimary": "sdbserver2:20000",
      "CausedBy": "ActiveLocation"
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
[rg_reelect]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/reelect.md
