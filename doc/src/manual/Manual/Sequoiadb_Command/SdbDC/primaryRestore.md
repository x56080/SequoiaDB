##名称##

primaryRestore - 根据保存的主节点信息恢复复制组的主节点

##语法##

**SdbDC.primaryRestore(\<planobjOrFile\>, [option])**

##类别##

SdbDC

##描述##

该函数用于根据 [primarySave()][primarySave] 保存的主节点信息，恢复各复制组的主节点。可以传入 primarySave 返回的 BSON 对象、普通 JSON 对象或保存结果的文件路径。

##参数##

planobjOrFile（ *object | BSONObj | string，必填* ）

指定主节点恢复计划，支持以下类型：

- object / BSONObj：[primarySave()][primarySave] 返回的结果对象，必须包含 Detail 数组
- string：[primarySave()][primarySave] 保存结果的文件路径，函数会从文件中读取并解析为 JSON 对象

option（ *object，选填* ）

通过参数 option 可以过滤需要恢复的数据组，以及指定选主的额外参数：

- HostName（ *string* ）：按主机名过滤，仅恢复 PrimaryNode 所在主机名匹配的数据组

    格式：`HostName: "sdbserver"`

- Domain（ *string | array* ）：按域过滤，仅恢复属于指定域的数据组。可以指定单个域名或域名数组

    格式：`Domain: "mydomain"` 或 `Domain: ["domain1", "domain2"]`

- Seconds（ *number* ）：选举的超时时间，默认值为 30，单位为秒

    格式：`Seconds: 60`

- Level（ *number* ）：重选举等待级别，取值：[1, 3]，默认为 3

    - 1：等待当前写操作结束，并阻塞后续写操作
    - 2：等待写游标（大对象操作）结束
    - 3：等待事务结束

    格式：`Level: 3`

> **Note:**
>
> - planobjOrFile 中必须包含有效的 Detail 数组，每个元素需包含 GroupName 和 PrimaryNode 字段。
> - PrimaryNode 的格式为 hostname:svcname，函数会解析出 HostName 和 ServiceName 用于 reelect 操作。
> - 如果 Detail 中某个条目的 GroupName 或 PrimaryNode 缺失，该条目将被跳过。
> - HostName 过滤基于 PrimaryNode 中的主机名进行匹配；Domain 过滤基于数据组所属的域进行匹配。

##返回值##

函数执行成功时，返回一个 BSON 对象，包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| MatchedNum | number | 过滤后匹配的复制组数量 |
| SucceedNum | number | 成功恢复主节点的复制组数量 |
| IgnoredNum | number | 被忽略的复制组数量（主节点未发生变化） |
| FailedNum | number | 恢复失败的复制组数量 |
| FailedGroups | array | 恢复失败的复制组名称列表（当 FailedNum > 0 时返回） |

函数执行失败时，将抛异常并输出错误信息。

##错误##

`primaryRestore()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误或缺少 Detail 字段 | 检查参数类型是否正确，确认对象中包含有效的 Detail 数组 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.8.6 及以上版本

##示例##

**示例1：** 使用 primarySave 返回的对象恢复主节点

```lang-javascript
> var dc = db.getDC()
> var result = dc.primarySave({HostName: "sdbserver"})
> dc.primaryRestore(result)
{
  "MatchedNum": 3,
  "SucceedNum": 2,
  "IgnoredNum": 1,
  "FailedNum": 0
}
```

**示例2：** 从文件中恢复主节点

```lang-javascript
> var dc = db.getDC()
> dc.primarySave(null, "/tmp/primary.json")
> dc.primaryRestore("/tmp/primary.json")
{
  "MatchedNum": 4,
  "SucceedNum": 4,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例3：** 指定选主参数进行恢复

```lang-javascript
> var dc = db.getDC()
> dc.primaryRestore("/tmp/primary.json", {Seconds: 60, Level: 1})
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例4：** 按主机名过滤，仅恢复指定主机上的数据组

```lang-javascript
> var dc = db.getDC()
> dc.primaryRestore("/tmp/primary.json", {HostName: "sdbserver1"})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例5：** 按域过滤，仅恢复指定域中的数据组

```lang-javascript
> var dc = db.getDC()
> dc.primaryRestore("/tmp/primary.json", {Domain: "mydomain"})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0
}
```

**示例6：** 恢复时部分复制组失败的情况

```lang-javascript
> var dc = db.getDC()
> dc.primaryRestore("/tmp/primary.json")
{
  "MatchedNum": 4,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 2,
  "FailedGroups": ["db3", "db4"]
}
```

[^_^]:
     本文使用的所有引用及链接
[primarySave]:manual/Manual/Sequoiadb_Command/SdbDC/primarySave.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
