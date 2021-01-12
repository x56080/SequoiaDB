详细的访问计划中，IXSCAN 对应一个使用索引扫描的上下文对象，展示的信息如下：

| 字段名                            | 类型      | 描述                                                                                       |
| --------------------------------- | --------- | ------------------------------------------------------------------------------------------ |
| Operator                          | 字符串    | 操作符的名称： "IXSCAN"                                                                    |
| Collection                        | 字符串    | IXSCAN 访问集合的名字                                                                      |
| Index                             | 字符串    | IXSCAN 访问索引的名字                                                                      |
| IXBound                           | BSON 对象 | IXSCAN 访问索引的查找范围                                                                  |
| Query                             | BSON 对象 | IXSCAN 执行的匹配符                                                                        |
| NeedMatch                         | 布尔型    | IXSCAN 是否需要在数据上执行匹配符进行过滤                                                  |
| IndexCover                        | 布尔型    | 访问计划匹配条件字段、选择字段、排序字段是否被索引覆盖。<br>被索引覆盖可以直接使用索引键值替代集合记录，提升访问性能 |
| Selector                          | BSON 对象 | IXSCAN 执行的选择符                                                                        |
| Skip                              | 长整型    | 指定 IXSCAN 需要跳过的记录个数                                                             |
| Return                            | 长整型    | 指定 IXSCAN 最多返回的记录个数                                                             |
| Estimate                          | BSON 对象 | 估算的 IXSCAN 代价信息<br>Estimate 选项为 true 时显示                                      |
| Estimate.StartCost                | 浮点型    | 估算的 IXSCAN 的启动时间（单位：秒）                                                       |
| Estimate.RunCost                  | 浮点型    | 估算的 IXSCAN 的运行时间（单位：秒）                                                       |
| Estimate.TotalCost                | 浮点型    | 估算的 IXSCAN 的结束时间（单位：秒）                                                       |
| Estimate.CLEstFromStat            | 布尔型    | IXSCAN 是否使用集合的统计信息进行估算                                                      |
| Estimate.CLStatTime               | 时间戳    | IXSCAN 使用的集合的统计信息的生成时间                                                      |
| Estimate.IXEstFromStat            | 布尔型    | IXSCAN 是否使用索引的统计信息进行估算                                                      |
| Estimate.IXStatTime               | 时间戳    | IXSCAN 使用的索引的统计信息的生成时间                                                      |
| Estimate.Input                    | BSON 对象 | 估算的 IXSCAN 输入的统计信息<br>Filter 选项包含 "Input" 时显示                             |
| Estimate.Input.Pages              | 长整型    | 估算的 IXSCAN 输入的数据页数                                                               |
| Estimate.Input.Records            | 长整型    | 估算的 IXSCAN 输入的记录个数                                                               |
| Estimate.Input.RecordSize         | 整型      | 估算的 IXSCAN 输入的记录平均字节数                                                         |
| Estimate.Input.IndexPages         | 整型      | 估算的 IXSCAN 输入的索引页数                                                               |
| Estimate.Filter                   | BSON 对象 | 估算的 IXSCAN 进行过滤的信息<br>Filter 选项包含 "Filter" 时显示                            |
| Estimate.Filter.MthSelectivity    | 浮点型    | 估算的 IXSCAN 使用匹配符进行过滤的选择率                                                   |
| Estimate.Filter.IXScanSelectivity | 浮点型    | 估算的 IXSCAN 使用索引时需要扫描索引的比例                                                 |
| Estimate.Filter.IXPredSelectivity | 浮点型    | 估算的 IXSCAN 使用索引进行过滤的选择率                                                     |
| Estimate.Output                   | BSON 对象 | 估算的 IXSCAN 输出的统计信息<br>Filter 选项包含 "Output" 时显示                            |
| Estimate.Output.Records           | 长整型    | 估算的 IXSCAN 输出的记录个数                                                               |
| Estimate.Output.RecordSize        | 整型      | 估算的 IXSCAN 输出的记录平均字节数                                                         |
| Estimate.Output.Sorted            | 布尔型    | IXSCAN 输出是否有序<br>如果索引包含 Sort 的所有字段并且匹配顺序，该项为 true，否则为 false |
| Run                               | BSON 对象 | 实际查询的 IXSCAN 代价信息<br>Run 选项为 true 时显示                                       |
| Run.ContextID                     | 长整型    | IXSCAN 执行的上下文 ID                                                                     |
| Run.StartTimestamp                | 字符串    | IXSCAN 启动的时间                                                                          |
| Run.QueryTimeSpent                | 浮点型    | IXSCAN 耗时（单位：秒）                                                                    |
| Run.GetMores                      | 长整型    | 请求 IXSCAN 返回结果集的次数                                                               |
| Run.ReturnNum                     | 长整型    | IXSCAN 返回记录个数                                                                        |
| Run.ReadRecords                   | 长整型    | IXSCAN 扫描数据记录个数                                                                    |
| Run.IndexReadRecords              | 长整型    | IXSCAN 扫描索引项个数                                                                      |

**示例**

```lang-json
{
  ...,
  "PlanPath": {
    "Operator": "IXSCAN",
    "Collection": "foo.bar",
    "Index": "index",
    "IXBound": {
      "a": [
        [
          1,
          1
        ]
      ]
    },
    "Query": {
      "$and": [
        {
          "a": {
            "$et": 1
          }
        }
      ]
    },
    "NeedMatch": false,
    "IndexCover": true,
    "Selector": {},
    "Skip": 0,
    "Return": -1,
    "Estimate": {
      "StartCost": 5e-7,
      "RunCost": 0.3200035,
      "TotalCost": 0.320004,
      "CLEstFromStat": false,
      "IXEstFromStat": false,
      "Input": {
        "Pages": 25,
        "Records": 25000,
        "RecordSize": 43,
        "IndexPages": 15
      },
      "Filter": {
        "MthSelectivity": 0.00004,
        "IXScanSelectivity": 0.00004,
        "IXPredSelectivity": 0.00004,
      },
      "Output": {
        "Records": 1,
        "RecordSize": 43,
        "Sorted": false
      }
    },
    "Run": {
      "ContextID": 36136,
      "StartTimestamp": "2017-12-11-16.11.34.518111",
      "QueryTimeSpent": 0.935198,
      "GetMores": 1,
      "ReturnNum": 5,
      "ReadRecords": 5,
      "IndexReadRecords": 6
    }
  }
}
```
