详细的访问计划中，MERGE 对象对应一个数据节点上的主表查询上下文对象，其中展示的信息如下：

| 字段名                           | 类型      | 描述                                                                         |
| -------------------------------- | --------- | ---------------------------------------------------------------------------- |
| Operator                         | 字符串    | 操作符的名称： "MERGE"                                                       |
| Sort                             | 字符串    | MERGE 需要保证输出结果有序的排序字段                                         |
| NeedReorder                      | 布尔型    | MERGE 是否需要根据排序字段对多个子表的记录进行排序合并<br>当查询中包含排序，排序字段不包含主表的分区键时为 true |
| SubCollectionNum                 | 整型      | MERGE 涉及查询的子表个数                                                     |
| SubCollectionList                | 数组      | MERGE 涉及查询的子表，按查询的执行顺序列出                                   |
| SubCollectionList.Name           | 字符串    | MERGE 发送查询的子表名称                                                     |
| SubCollectionList.EstTotalCost   | 浮点数    | MERGE 发送的查询在子表上查询的估算时间（单位：秒）                           |
| SubCollectionList.QueryTimeSpent | 浮点数    | MERGE 发送的查询在子表上查询的执行时间（单位：秒）<br>Run 选项为 true 时显示 |
| SubCollectionList.WaitTimeSpent  | 浮点数    | MERGE 发送的查询在数据节点上查询的等待时间（单位：秒）<br>Run 选项为 true 时显示） |
| Selector                         | BSON 对象 | MERGE 执行的选择符                                                           |
| Skip                             | 长整型    | 指定 MERGE 需要跳过的记录个数                                                |
| Return                           | 长整型    | 指定 MERGE 最多返回的记录个数                                                |
| Estimate                         | BSON 对象 | 估算的 MERGE 代价信息<br>Estimate 选项为 true 时显示                         |
| Estimate.StartCost               | 浮点型    | 估算的 MERGE 的启动时间（单位：秒）                                          |
| Estimate.RunCost                 | 浮点型    | 估算的 MERGE 的运行时间（单位：秒）                                          |
| Estimate.TotalCost               | 浮点型    | 估算的 MERGE 的结束时间（单位：秒）                                          |
| Estimate.Output                  | BSON 对象 | 估算的 MERGE 输出结果的统计信息<br>Filter 选项包含 "Output" 时显示           |
| Estimate.Output.Records          | 长整型    | 估算的 MERGE 输出的记录个数                                                  |
| Estimate.Output.RecordSize       | 整型      | 估算的 MERGE 输出的记录平均字节数                                            |
| Estimate.Output.Sorted           | 布尔型    | MERGE 输出结果是否有序                                                       |
| Run                              | BSON 对象 | 实际执行 MERGE 的代价信息<br>Run 选项为 true 时显示                          |
| Run.ContextID                    | 长整型    | MERGE 执行的上下文 ID                                                        |
| Run.StartTimestamp               | 字符串    | MERGE 执行启动的时间戳                                                       |
| Run.QueryTimeSpent               | 浮点型    | MERGE 执行耗时（单位：秒）                                                   |
| Run.GetMores                     | 长整型    | 请求 MERGE 返回结果集的次数                                                  |
| Run.ReturnNum                    | 长整型    | MERGE 返回记录个数                                                           |
| SubCollections                   | 数组      | MERGE 的子操作（每个子表返回的查询的访问计划结果）<br>详细请参考：[数据节点的访问计划](database_management/access_plans/detailed_access_plan/collection_plan.md) |

**示例**

```lang-json
{
  ...,
  "PlanPath": {
    "Operator": "MERGE",
    "Sort": {
      "h": 1
    },
    "NeedReorder": true,
    "SubCollectionNum": 2,
    "SubCollectionList": [
      {
        "Name": "subcs.subcl1",
        "EstTotalCost": 0.8277414999999999,
        "QueryTimeSpent": 1.080046,
        "WaitTimeSpent": 0.000234
      },
      {
        "Name": "subcs.subcl2",
        "EstTotalCost": 0.8277414999999999,
        "QueryTimeSpent": 0.946832,
        "WaitTimeSpent": 0.000182
      }
    ],
    "Selector": {},
    "Skip": 0,
    "Return": -1,
    "Estimate": {
      "StartCost": 1.630483,
      "RunCost": 0.09999999999999999,
      "TotalCost": 1.730483,
      "Output": {
        "Records": 50000,
        "RecordSize": 43,
        "Sorted": true
      }
    },
    "Run": {
      "ContextID": 63121,
      "StartTimestamp": "2017-12-11-16.18.00.789234",
      "QueryTimeSpent": 1.203218,
      "GetMores": 3,
      "ReturnNum": 50000
    },
    "SubCollections": [
      {
        ...
      },
      {
        ...
      }
    ]
  }
}
```
