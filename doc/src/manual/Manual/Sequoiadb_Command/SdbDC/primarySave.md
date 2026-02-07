##名称##

primarySave - 保存集群中符合条件的复制组主节点信息

##语法##

**SdbDC.primarySave([option], [filename])**

##类别##

SdbDC

##描述##

该函数用于获取集群中符合条件的复制组的主节点信息，以对象方式返回。如果指定了文件名，还会同时将结果保存到文件中。保存的结果可以用于 [primaryRestore()][primaryRestore] 恢复主节点。

##参数##

option（ *object，选填* ）

通过参数 option 可以指定过滤条件，为 null 或不指定时获取所有复制组的主节点信息：

- HostName（ *string* ）：指定主机名进行过滤

    筛选出包含指定主机名节点的复制组。

    格式：`HostName: "sdbserver"`

- Domain（ *string | string array* ）：指定域进行过滤

    在通过 HostName 筛选的基础上，进一步按域过滤复制组。仅获取属于指定域的复制组的主节点信息。

    格式：`Domain: "mydomain"` 或 `Domain: ["domain1", "domain2"]`

filename（ *string，选填* ）

指定保存结果的文件路径。如果指定，结果将以 JSON 格式写入该文件。

> **Note:**
>
> - 协调节点组（SYSCoord）和备用组（SYSSpare）不包含在结果中。
> - 如果指定的 Domain 不存在或域中没有复制组，将抛出异常。
> - 如果复制组没有主节点，该组将被计入失败。

##返回值##

函数执行成功时，返回一个 BSON 对象，包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| MatchedNum | number | 匹配的复制组数量 |
| SucceedNum | number | 成功获取主节点信息的复制组数量 |
| IgnoredNum | number | 被忽略的复制组数量 |
| FailedNum | number | 获取失败的复制组数量（没有主节点或节点不可达） |
| FailedGroups | array | 获取失败的复制组名称列表（当 FailedNum > 0 时返回） |
| Detail | array | 主节点详细信息列表 |

Detail 数组中每个元素包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| GroupName | string | 复制组名称 |
| PrimaryNode | string | 主节点地址，格式为 hostname:svcname |

函数执行失败时，将抛异常并输出错误信息。

##错误##

`primarySave()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误 | 检查参数类型是否正确 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.8.6 及以上版本

##示例##

**示例1：** 获取所有复制组的主节点信息

```lang-javascript
> var dc = db.getDC()
> dc.primarySave()
{
  "MatchedNum": 4,
  "SucceedNum": 4,
  "IgnoredNum": 0,
  "FailedNum": 0,
  "Detail": [
    {
      "GroupName": "SYSCatalogGroup",
      "PrimaryNode": "sdbserver:30020"
    },
    {
      "GroupName": "db1",
      "PrimaryNode": "sdbserver:20010"
    },
    {
      "GroupName": "db2",
      "PrimaryNode": "sdbserver:42000"
    },
    {
      "GroupName": "db3",
      "PrimaryNode": "sdbserver:21020"
    }
  ]
}
```

**示例2：** 按主机名过滤并保存到文件

```lang-javascript
> var dc = db.getDC()
> dc.primarySave({HostName: "sdbserver"}, "/tmp/primary.json")
{
  "MatchedNum": 3,
  "SucceedNum": 3,
  "IgnoredNum": 0,
  "FailedNum": 0,
  "Detail": [
    {
      "GroupName": "db1",
      "PrimaryNode": "sdbserver:20010"
    },
    {
      "GroupName": "db2",
      "PrimaryNode": "sdbserver:42000"
    },
    {
      "GroupName": "db3",
      "PrimaryNode": "sdbserver:21020"
    }
  ]
}
```

**示例3：** 使用 null 作为 option 并保存到文件

```lang-javascript
> var dc = db.getDC()
> dc.primarySave(null, "/tmp/primary.json")
```

**示例4：** 按域过滤

```lang-javascript
> var dc = db.getDC()
> dc.primarySave({Domain: "mydomain"})
{
  "MatchedNum": 2,
  "SucceedNum": 2,
  "IgnoredNum": 0,
  "FailedNum": 0,
  "Detail": [
    {
      "GroupName": "db1",
      "PrimaryNode": "sdbserver:20010"
    },
    {
      "GroupName": "db2",
      "PrimaryNode": "sdbserver:42000"
    }
  ]
}
```

[^_^]:
     本文使用的所有引用及链接
[primaryRestore]:manual/Manual/Sequoiadb_Command/SdbDC/primaryRestore.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
