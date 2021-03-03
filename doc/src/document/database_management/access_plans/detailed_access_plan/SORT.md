详细的访问计划中，SORT 对象对应一个数据节点上的排序上下文对象，其中展示的信息如下：

| 字段名                     | 类型      | 描述                                                                          |
| -------------------------- | --------- | ----------------------------------------------------------------------------- |
| Operator                   | 字符串    | 操作符的名称： "SORT"                                                         |
| Sort                       | BSON 对象 | SORT 执行的排序字段                                                           |
| Selector                   | BSON 对象 | SORT 执行的选择符                                                             |
| Skip                       | 长整型    | 指定 SORT 需要跳过的记录个数                                                  |
| Return                     | 长整型    | 指定 SORT 最多返回的记录个数                                                  |
| Estimate                   | BSON 对象 | 估算的 SORT 代价信息<br>Estimate 选项为 true 时显示                           |
| Estimate.StartCost         | 浮点型    | 估算的 SORT 的启动时间（单位：秒）                                            |
| Estimate.RunCost           | 浮点型    | 估算的 SORT 的运行时间（单位：秒）                                            |
| Estimate.TotalCost         | 浮点型    | 估算的 SORT 的结束时间（单位：秒）                                            |
| Estimate.SortType          | 字符串    | SORT 估算的排序类型：<br>1. "InMemory" 为内存排序<br>2. "External" 为外存排序 |
| Estimate.Output            | BSON 对象 | 估算的 SORT 输出的统计信息<br>Filter 选项包含 "Output" 时显示                 |
| Estimate.Output.Records    | 长整型    | 估算的 SORT 输出的记录个数                                                    |
| Estimate.Output.RecordSize | 整型      | 估算的 SORT 输出的记录平均字节数                                              |
| Estimate.Output.Sorted     | 布尔型    | SORT 输出是否有序，对 SORT 为 true                                            |
| Run                        | BSON 对象 | 实际查询的 SORT 代价信息<br> Run 选项为 true 时显示                           |
| Run.ContextID              | 长整型    | SORT 执行的上下文 ID                                                          |
| Run.StartTimestamp         | 字符串    | SORT 启动的时间                                                               |
| Run.QueryTimeSpent         | 浮点型    | SORT 耗时（单位：秒）                                                         |
| Run.GetMores               | 长整型    | 请求 SORT 返回结果集的次数                                                    |
| Run.ReturnNum              | 长整型    | SORT 返回记录个数                                                             |
| Run.SortType               | 字符串    | SORT 执行的排序类型：<br>1. "InMemory" 为内存排序<br>2. "External" 为外存排序 |
| ChildOperators             | 数组      | SORT 的子操作（[TBSCAN](database_management/access_plans/detailed_access_plan/TBSCAN.md) 或 [IXSCAN](database_management/access_plans/detailed_access_plan/IXSCAN.md)） |

**示例**

```lang-json
{
  ...,
  "PlanPath": {
    "Operator": "SORT",
    "Sort": {
      "c": 1
    },
    "Selector": {},
    "Skip": 0,
    "Return": -1,
    "Estimate": {
      "StartCost": 0.475,
      "RunCost": 5e-7,
      "TotalCost": 0.4750005,
      "SortType": "InMemory",
      "Output": {
        "Records": 1,
        "RecordSize": 43,
        "Sorted": true
      }
    },
    "Run": {
      "ContextID": 8,
      "StartTimestamp": "2017-11-29-14.02.38.108504",
      "QueryTimeSpent": 0.050564,
      "GetMores": 1,
      "ReturnNum": 5,
      "SortType": "InMemory"
    },
    "ChildOperators": [
      {
        ...
      }
    ]
  }
}
```
