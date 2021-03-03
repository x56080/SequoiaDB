详细的访问计划中，COORD-MERGE 对象对应一个协调节点上的查询上下文对象，其中展示的信息如下：

| 字段名                      | 类型      | 描述                                                                     |
| --------------------------- | --------- | ------------------------------------------------------------------------ |
| Operator                    | 字符串    | 操作符的名称： "COORD-MERGE"                                             |
| Sort                        | 字符串    | COORD-MERGE 需要保证输出结果有序的排序字段                               |
| NeedReorder                 | 布尔型    | COORD-MERGE 是否需要根据排序字段对多个数据组的记录进行排序合并<br>当查询中包含排序的时候 NeedReorder 为 true |
| DataNodeNum                 | 整型      | COORD-MERGE 涉及查询的数据节点个数                                       |
| DataNodeList                | 数组      | COORD-MERGE 涉及查询的数据节点，按查询的执行顺序列出                     |
| DataNodeList.Name           | 字符串    | COORD-MERGE 发送查询的数据节点名称                                       |
| DataNodeList.EstTotalCost   | 浮点数    | COORD-MERGE 发送的查询在该数据节点上查询的估算时间（单位：秒）           |
| DataNodeList.QueryTimeSpent | 浮点数    | 在数据节点上查询的执行时间（单位：秒）<br>Run 选项为 true 时显示         |
| DataNodeList.WaitTimeSpent  | 浮点数    | COORD-MERGE 发送的查询在数据节点上查询的等待时间（单位：秒）<br>Run 选项为 true 时显示 |
| Selector                    | BSON 对象 | COORD-MERGE 执行的选择符                                                 |
| Skip                        | 长整型    | 指定 COORD-MERGE 需要跳过的记录个数                                      |
| Return                      | 长整型    | 指定 COORD-MERGE 最多返回的记录个数                                      |
| Estimate                    | BSON 对象 | 估算的 COORD-MERGE 代价信息<br>Estimate 选项为 true 时显示               |
| Estimate.StartCost          | 浮点型    | 估算的 COORD-MERGE 的启动时间（单位：秒）                                |
| Estimate.RunCost            | 浮点型    | 估算的 COORD-MERGE的运行时间（单位：秒）                                 |
| Estimate.TotalCost          | 浮点型    | 估算的 COORD-MERGE 的结束时间（单位：秒）                                |
| Estimate.Output             | BSON 对象 | 估算的 COORD-MERGE 输出结果的统计信息<br>Filter 选项包含 "Output" 时显示 |
| Estimate.Output.Records     | 长整型    | 估算的 COORD-MERGE 输出的记录个数                                        |
| Estimate.Output.RecordSize  | 整型      | 估算的 COORD-MERGE 输出的记录平均字节数                                  |
| Estimate.Output.Sorted      | 布尔型    | COORD-MERGE 输出结果是否有序                                             |
| Run                         | BSON 对象 | 实际执行 COORD-MERGE 的代价信息<br>Run 选项为 true 时显示                |
| Run.ContextID               | 长整型    | COORD-MERGE 执行的上下文 ID                                              |
| Run.StartTimestamp          | 字符串    | COORD-MERGE 执行启动的时间戳                                             |
| Run.QueryTimeSpent          | 浮点型    | COORD-MERGE 执行耗时（单位：秒）                                         |
| Run.GetMores                | 长整型    | 请求 COORD-MERGE 返回结果集的次数                                        |
| Run.ReturnNum               | 长整型    | COORD-MERGE 返回记录个数                                                 |
| Run.WaitTimeSpent           | 浮点型    | COORD-MERGE 等待数据返回的时间（单位：秒，以秒为单位的粗略统计信息）     |
| ChildOperators              | 数组      | COORD-MERGE 的子操作（每个数据组返回的查询的访问计划结果）<br>详细请参考：[主表的访问计划](database_management/access_plans/detailed_access_plan/main_collection_plan.md) 或者 [数据节点的访问计划](database_management/access_plans/detailed_access_plan/collection_plan.md) |

**示例**

```lang-json
{
  ...,
  "PlanPath": {
    "Operator": "COORD-MERGE",
    "Sort": {},
    "NeedReorder": false,
    "DataNodeNum": 2,
    "DataNodeList": [
      {
        "Name": "hostname:11820",
        "EstTotalCost": 0.4750005,
        "QueryTimeSpent": 0.045813,
        "WaitTimeSpent": 0.000124
      },
      {
        "Name": "hostname:11830",
        "EstTotalCost": 0.4750005,
        "QueryTimeSpent": 0.045841,
        "WaitTimeSpent": 0.000108
      }
    ],
    "Selector": {},
    "Skip": 0,
    "Return": -1,
    "Estimate": {
      "StartCost": 0,
      "RunCost": 0.4750015,
      "TotalCost": 0.4750015,
      "Output": {
        "Records": 2,
        "RecordSize": 43,
        "Sorted": false
      }
    },
    "Run": {
      "ContextID": 9,
      "StartTimestamp": "2017-12-09-13.51.14.749863",
      "QueryTimeSpent": 0.046311,
      "GetMores": 3,
      "ReturnNum": 10,
      "WaitTimeSpent": 0
    },
    "ChildOperators": [
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
