##名称##

locationAnalyze - 分析集群中 Location 的分布合理性

##语法##

**SdbDC.locationAnalyze([option], [fileName])**

##类别##

SdbDC

##描述##

该函数用于分析集群中 Location 的分布是否合理。通过查询 SDB_LIST_GROUPS 和 SDB_LIST_GROUPMODES，解析各节点的 Location 设置、ActiveLocation 状态以及 GroupMode（Critical/Maintenance）状态，输出各 Location 的详细分析信息和异常信息。

##参数##

option（ *object，选填* ）

通过参数 option 可以指定过滤条件，为 null 或不指定时分析所有组（包括协调组和数据组）：

- HostName（ *string* ）：按主机名过滤，仅分析包含指定主机名节点的组

    格式：`HostName: "sdbserver"`

- Domain（ *string | string array* ）：按域过滤，仅分析属于指定域的组。可以指定单个域名或域名数组

    格式：`Domain: "mydomain"` 或 `Domain: ["domain1", "domain2"]`

fileName（ *string，选填* ）

指定输出文件路径。指定后，分析结果将以 JSON 格式（2 空格缩进）写入该文件。


##返回值##

函数执行成功时，返回一个 BSON 对象，包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| MatchedHostNum | number | 匹配的主机数量 |
| MatchedGroupNum | number | 匹配的组数量（包括协调组） |
| MatchedNodeNum | number | 匹配的节点数量 |
| ActiveLocation | string / array | 所有数据组的 ActiveLocation。如果所有数据组的 ActiveLocation 相同，返回字符串；如果不同，返回排序后的数组；如果没有设置，返回空字符串 |
| LocationInfo | array | Location 详细信息列表 |
| ExceptionHostInfo | object | 主机异常信息（存在异常时返回） |
| ExceptionGroupInfo | object | 组异常信息（存在异常时返回） |

LocationInfo 数组中每个元素包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| LocationName | string | Location 名称 |
| ActiveStatus | string | ActiveLocation 状态，取值："All"（所有数据组的 ActiveLocation 都是该 Location）、"None"（没有数据组的 ActiveLocation 是该 Location）、"Partial"（部分数据组的 ActiveLocation 是该 Location） |
| GroupStatus | string | GroupMode 状态，取值见下表 |
| WholeHost | array | 该 Location 覆盖了主机上所有匹配节点的主机名列表 |
| PartialHost | array | 该 Location 仅覆盖了主机上部分匹配节点的主机信息列表，每个元素包含 HostName 和 Node 字段（存在时返回） |

GroupStatus 取值说明：

| 取值 | 描述 |
| ---- | ---- |
| "" | 该 Location 所在的数据组均未设置 GroupMode |
| "Critical" | 该 Location 所在的所有数据组都对该 Location 设置了 Critical 模式，且每个组中该 Location 的所有节点都被覆盖 |
| "PartialCritical" | 该 Location 所在的数据组中存在 Critical 模式，但未满足 "Critical" 的条件（部分组未设置，或某些组中仅覆盖了部分节点） |
| "Maintenance" | 该 Location 所在的所有数据组都对该 Location 的所有节点设置了 Maintenance 模式 |
| "PartialMaintenance" | 该 Location 所在的数据组中存在 Maintenance 模式，但未满足 "Maintenance" 的条件（部分组未设置，或某些组中仅覆盖了部分节点） |
| "Critical-Maintenance" | 该 Location 所在的所有数据组中，部分设置了 Critical 模式，部分设置了 Maintenance 模式，且每个组中该 Location 的所有节点都被覆盖 |
| "Partial-Critical-Maintenance" | 该 Location 所在的数据组中，存在 Critical 和 Maintenance 模式，但未满足 "Critical-Maintenance" 的条件 |

ExceptionHostInfo 包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| NoLocationHost | array | 所有节点均未设置 Location 的主机名列表 |
| PartialLocationHost | array | 部分节点设置了 Location、部分节点未设置 Location 的主机名列表 |
| MultiLocationHost | array | 节点分布在多个不同 Location 的主机名列表 |

ExceptionGroupInfo 包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| NoLocationGroup | array | 所有节点均未设置 Location 的组名列表 |
| PartialLocationGroup | array | 部分节点设置了 Location、部分节点未设置 Location 的组名列表 |
| OneLocationGroup | array | 所有设置了 Location 的节点都在同一个 Location 的组名列表 |

函数执行失败时，将抛异常并输出错误信息。

##错误##

`locationAnalyze()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误 | 检查参数类型是否正确 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.8.6 及以上版本

##示例##

**示例1：** 分析所有组的 Location 分布

```lang-javascript
> var dc = db.getDC()
> dc.locationAnalyze()
{
  "MatchedHostNum": 3,
  "MatchedGroupNum": 4,
  "MatchedNodeNum": 12,
  "ActiveLocation": "GuangZhou",
  "LocationInfo": [
    {
      "LocationName": "GuangZhou",
      "ActiveStatus": "All",
      "GroupStatus": "",
      "WholeHost": [
        "sdbserver1"
      ],
      "PartialHost": [
        {
          "HostName": "sdbserver2",
          "Node": [
            "sdbserver2:20000",
            "sdbserver2:20010"
          ]
        }
      ]
    },
    {
      "LocationName": "ShenZhen",
      "ActiveStatus": "None",
      "GroupStatus": "Critical",
      "WholeHost": [],
      "PartialHost": [
        {
          "HostName": "sdbserver2",
          "Node": [
            "sdbserver2:20020"
          ]
        },
        {
          "HostName": "sdbserver3",
          "Node": [
            "sdbserver3:20000",
            "sdbserver3:20010"
          ]
        }
      ]
    }
  ],
  "ExceptionHostInfo": {
    "NoLocationHost": [],
    "PartialLocationHost": [
      "sdbserver3"
    ],
    "MultiLocationHost": [
      "sdbserver2"
    ]
  },
  "ExceptionGroupInfo": {
    "NoLocationGroup": [],
    "PartialLocationGroup": [
      "db3"
    ],
    "OneLocationGroup": [
      "db1"
    ]
  }
}
```

**示例2：** 按域过滤并将结果保存到文件

```lang-javascript
> var dc = db.getDC()
> dc.locationAnalyze({Domain: "mydomain"}, "/tmp/location_report.json")
{
  "MatchedHostNum": 2,
  "MatchedGroupNum": 2,
  "MatchedNodeNum": 6,
  "ActiveLocation": "GuangZhou",
  "LocationInfo": [
    {
      "LocationName": "GuangZhou",
      "ActiveStatus": "All",
      "GroupStatus": "",
      "WholeHost": [
        "sdbserver1"
      ]
    }
  ],
  "ExceptionGroupInfo": {
    "NoLocationGroup": [],
    "PartialLocationGroup": [],
    "OneLocationGroup": [
      "db1"
    ]
  }
}
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
