SequoiaDB在查询优化中使用的统计信息，包含集合和索引的数据分布信息。查询优化器可以根据这些统计信息来估计查询结果中的基数，从而创建高质量的查询计划。

在SequoiaDB中有两种统计信息，集合的统计信息和索引的统计信息。

##集合的统计信息##

集合的统计信息存放在数据节点的SYSSTAT.SYSCOLLECTIONSTAT集合中，具体的字段如下：

| 字段名 | 数据类型 | 默认值 | 必须 | 说明 |
| ---- | ---- | ---- | ---- | ---- |
| CollectionSpace | String | | 是 | 统计的Collection所在Collection Space的名称 |
| Collection | String | | 是 | 统计的Collection的名称（不带Collection Space名字） |
| CreateTime | NumberLong | 0 | 是 | 统计收集的时间戳 |
| SampleRecords | NumberLong | 0 | 是 | 统计收集时抽样的文档个数 |
| TotalRecords | NumberLong | 10 | 是 | 统计收集时的文档个数 |
| TotalDataPages | NumberInt | 1 | 是 | 统计收集时的数据页个数 |
| TotalDataSize | NumberLong | | 是 | 统计收集时的数据总大小（字节数） |
| AvgNumFields | NumberInt | 10 | 否 | 文档的平均字段数 |

例子：

```lang-json
{
  "Collection": "foo",
  "CollectionSpace": "bar",
  "CreateTime": 1496910925978,
  "SampleRecords": 200,
  "TotalDataPages": 1284,
  "TotalDataSize": 65929411,
  "TotalRecords": 600000,
  "AvgNumFields" : 10
}
```

##索引的统计信息##

索引的统计信息存放在数据节点的SYSSTAT.SYSINDEXSTAT集合中，具体的字段如下：

| 字段名 | 数据类型 | 默认值 | 必须 | 说明 |
| ---- | ---- | ---- | ---- | ---- |
| CollectionSpace | String | | 是 | 统计的Collection所在Collection Space的名称 |
| Collection | String | | 是 | 统计的Collection的名称（不带Collection Space名字） |
| CreateTime | NumberLong | 0 | 是 | 统计收集的时间戳 |
| Index | String | | 是 | 统计Index的名称 |
| KeyPattern | BSONObj | | 是 | 统计索引的字段定义，例如：{a:1, b:-1} |
| SampleRecords | NumberLong | 0 | 是 | 统计收集时抽样的文档个数 |
| TotalRecords | NumberLong | 10 | 是 | 统计收集时的文档个数 |
| IndexPages | NumberInt | 1 | 是 | 统计收集时索引的页个数 |
| IndexLevels | NumberInt | 1 | 是 | 统计收集时索引的层数 |
| IsUnique | BOOL | FALSE | 是 | Index是否唯一索引 |
| MCV | Object | undefined | 否 | 频繁数值集合(Most Common Values) <br/>如：MCV: { Values: [ {a:1,b:1}, {a:2, b:2}, ... ], Frac: [ 1000, 1000, ... ] } |
| MCV.Values | Array | | 是(如有MCV) | 频繁数值的值 |
| MCV.Frac | Array | | 是(如有MCV) | 频繁数值的比例，每个值的取值 0 ~ 10000，最终比例为 (Frac / 10000) * 100% |

```lang-json
{

  "Collection": "foo",
  "CollectionSpace": "bar",
  "CreateTime": 1496910926035,
  "Index": "index",
  "IndexLevels": 2,
  "IndexPages": 256,
  "IsUnique": false,
  "KeyPattern": {
    "a": 1
  },
  "MCV": {
    "Values": [
      {
        "a": 2358
      },
      {
        "a": 7074
      },
      {
        "a": 11790
      },
      ...
    ],
    "Frac": [
      50,
      50,
      50,
      ...
    ]
  },
  "SampleRecords": 200,
  "TotalRecords": 600000
}
```

##统计信息的使用##

统计信息可以用于查询优化器评估索引的选择率，参考 [基于代价的访问计划评估](database_management/access_plans/search_paths/overview.md)。

**相等比较的选择率估算**

1.  如果字段上建立的是唯一索引，则选择率为：```selectivity = 1 / TotalRecords```

2.  如果相等比较的值落入频繁数值集合中，假设命中下标为 i，则选择率为：```selectivity = MCV.Frac[i]```

3.  如果相等比较的值没有落入频繁数值集合中，则选择率为：```selectivity = ( 1 - sum( MCV.Frac ) ) * 0.005```

**范围比较的选择率估算**

1.  如果相等比较的范围落入频繁数值集合中，假设命中下标为 m 至 n，则选择率为：```selectivity = MCV.Frac[m] + ... + MCV.Frac[n]```

2.  如果相等比较的范围没有落入频繁数值集合中，则选择率为：```selectivity = ( 1 - sum( MCV.Frac ) ) * 0.05```

**示例**

统计信息中的字段 "val" 的频繁数值集合的内容为：

```lang-json
MCV : {
  Val : [
    1, 2, 3, 4, 5, 6, 7, 8, 9
  ],
  Frac : [
    1000, 1200, 800, 1300, 700, 1000, 1000, 1000, 1000
  ]
}
```

1.  ```{ val : { $et : 1 } }``` 命中频繁数值集合，因此其选择率估算为：```selectivity = 0.1```
2.  ```{ val : { $et : 10 } }``` 没有命中频繁数值集合，因此其选择率估算为：```selectivity = 0.1 * 0.005 = 0.0005```
3.  ```{ val : { $lt : 4 } }``` 命中了频繁数值集合的下标0、1 和 2，因此其选择率估算为：```selectivity = 0.1 + 0.12 + 0.08 = 0.2```

>   **Note:**
>
>   频繁数值的比例，每个值的取值 0 ~ 10000，最终比例为 (Frac / 10000) * 100%

##统计信息的收集##

请参考[db.analyze\(\)](reference/Sequoiadb_command/Sdb/analyze.md)。
