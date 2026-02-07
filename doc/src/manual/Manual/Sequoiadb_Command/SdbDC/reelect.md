##名称##

reelect - 对集群中符合条件的复制组重新选主

##语法##

**SdbDC.reelect(\<option\>)**

##类别##

SdbDC

##描述##

该函数用于对集群中符合条件的复制组批量执行重新选主操作，可以通过指定主机名或位置集来筛选目标复制组，并可选通过域进一步过滤。

##参数##

option（ *object，必填* ）

通过参数 option 可以指定选主的条件：

- HostName（ *string* ）：指定主机名进行过滤（与 Location 互斥）

    筛选出包含指定主机名节点的复制组，并在这些复制组中重新选主。

    格式：`HostName: "sdbserver"`

- Location（ *string* ）：指定位置集进行过滤（与 HostName 互斥）

    筛选出包含指定位置集节点的复制组，并在这些复制组中重新选主。

    格式：`Location: "GuangZhou"`

- Domain（ *string | string array* ）：指定域进行过滤

    在通过 HostName 或 Location 筛选的基础上，进一步按域过滤复制组。仅对属于指定域的复制组执行选主操作。

    格式：`Domain: "mydomain"` 或 `Domain: ["domain1", "domain2"]`

- Seconds（ *number* ）：选举的超时时间，默认值为 30，单位为秒

    格式：`Seconds: 60`

- Level（ *number* ）：重选举等待级别，取值：[1, 3]，默认为 3

    - 1：等待当前写操作结束，并阻塞后续写操作
    - 2：等待写游标（大对象操作）结束
    - 3：等待事务结束

    格式：`Level: 3`

- Mode（ *number* ）：节点指定模式，默认为 1

    - 0：排除模式，排除指定的节点当选为主节点
    - 1：指定模式，指定的节点当选为主节点

    格式：`Mode: 1`

> **Note:**
>
> - HostName 和 Location 必须指定其中一个，且不能同时指定。
> - 协调节点组（SYSCoord）和备用组（SYSSpare）不参与选主操作。
> - 如果指定的 HostName 或 Location 在集群中不存在，将抛出异常。
> - 如果指定的 Domain 不存在或域中没有复制组，将抛出异常。
> - Seconds、Level、Mode 参数会传递给每个复制组的 [rg.reelect()][rg_reelect] 操作。

##返回值##

函数执行成功时，返回一个 BSON 对象，包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| MatchedNum | number | 匹配的复制组数量 |
| SucceedNum | number | 成功选主的复制组数量 |
| IgnoredNum | number | 被忽略的复制组数量（主节点未发生变化） |
| FailedNum | number | 选主失败的复制组数量 |
| FailedGroups | array | 选主失败的复制组名称列表（当 FailedNum > 0 时返回） |

函数执行失败时，将抛异常并输出错误信息。

##错误##

`reelect()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误或缺少必填参数 | 检查参数类型是否正确，确认已指定 HostName 或 Location |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.8.6 及以上版本

##示例##

**示例1：** 对指定主机上的复制组重新选主

```lang-javascript
> var dc = db.getDC()
> dc.reelect({HostName: "sdbserver"})
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例2：** 对指定位置集的复制组重新选主

```lang-javascript
> var dc = db.getDC()
> dc.reelect({Location: "GuangZhou"})
{
  "MatchedNum": 4,
  "SucceedNum": 2,
  "IgnoredNum": 2,
  "FailedNum": 0
}
```

**示例3：** 结合域进行过滤后重新选主

```lang-javascript
> var dc = db.getDC()
> dc.reelect({HostName: "sdbserver", Domain: "mydomain"})
{
  "MatchedNum": 2,
  "SucceedNum": 1,
  "IgnoredNum": 0,
  "FailedNum": 1,
  "FailedGroups": ["group2"]
}
```

**示例4：** 使用多个域进行过滤

```lang-javascript
> var dc = db.getDC()
> dc.reelect({Location: "GuangZhou", Domain: ["domain1", "domain2"]})
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
[rg_reelect]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/reelect.md
