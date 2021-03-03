详细的访问计划中，TBSCAN 对应一个使用全表扫描的上下文对象，展示的信息如下：

| 字段名                         | 类型      | 描述                                                            |
| ------------------------------ | --------- | --------------------------------------------------------------- |
| Operator                       | 字符串    | 操作符的名称： "TBSCAN"                                         |
| Collection                     | 字符串    | TBSCAN 访问的集合名字                                           |
| Query                          | BSON 对象 | TBSCAN 执行的匹配符                                             |
| Selector                       | BSON 对象 | TBSCAN 执行的选择符                                             |
| Skip                           | 长整型    | 指定 TBSCAN 需要跳过的记录个数                                  |
| Return                         | 长整型    | 指定 TBSCAN 最多返回的记录个数                                  |
| Estimate                       | BSON 对象 | 估算的 TBSCAN 代价信息<br>Estimate 选项为 true 时显示           |
| Estimate.StartCost             | 浮点型    | 估算的 TBSCAN 的启动时间（单位：秒）                            |
| Estimate.RunCost               | 浮点型    | 估算的 TBSCAN 的运行时间（单位：秒）                            |
| Estimate.TotalCost             | 浮点型    | 估算的 TBSCAN 的结束时间（单位：秒）                            |
| Estimate.CLEstFromStat         | 布尔型    | TBSCAN 是否使用集合的统计信息进行估算                           |
| Estimate.CLStatTime            | 字符串    | TBSCAN 使用的集合的统计信息的生成时间                           |
| Estimate.Input                 | BSON 对象 | 估算的 TBSCAN 输入的统计信息<br>Filter 选项包含 "Input" 时显示  |
| Estimate.Input.Pages           | 长整型    | 估算的 TBSCAN 输入的数据页数                                    |
| Estimate.Input.Records         | 长整型    | 估算的 TBSCAN 输入的记录个数                                    |
| Estimate.Input.RecordSize      | 整型      | 估算的 TBSCAN 输入的记录平均字节数                              |
| Estimate.Filter                | BSON 对象 | 估算的 TBSCAN 进行过滤的信息<br>Filter 选项包含 "Filter" 时显示 |
| Estimate.Filter.MthSelectivity | 浮点型    | 估算的 TBSCAN 使用匹配符进行过滤的选择率                        |
| Estimate.Output                | BSON 对象 | 估算的 TBSCAN 输出的统计信息<br>Filter 选项包含 "Output" 时显示 |
| Estimate.Output.Records        | 长整型    | 估算的 TBSCAN 输出的记录个数                                    |
| Estimate.Output.RecordSize     | 整型      | 估算的 TBSCAN 输出的记录平均字节数                              |
| Estimate.Output.Sorted         | 布尔型    | TBSCAN 输出是否有序，对 TBSCAN 为 false                         |
| Run                            | BSON 对象 | 实际执行 TBSCAN 的代价信息<br>Run 选项为 true 时显示            |
| Run.ContextID                  | 长整型    | TBSCAN 执行的上下文标识                                         |
| Run.StartTimestamp             | 字符串    | TBSCAN 执行启动的时间戳                                         |
| Run.QueryTimeSpent             | 浮点型    | TBSCAN 执行耗时（单位：秒）                                     |
| Run.GetMores                   | 长整型    | 请求 TBSCAN 返回结果集的次数                                    |
| Run.ReturnNum                  | 长整型    | TBSCAN 返回记录个数                                             |
| Run.ReadRecords                | 长整型    | TBSCAN 扫描记录个数                                             |

**示例**

```lang-json
{
  ...,
  "PlanPath": {
    "Operator": "TBSCAN",
    "Collection": "foo.bar",
    "Query": {
      "$and": []
    },
    "Selector": {},
    "Skip": 0,
    "Return": -1,
    "Estimate": {
      "StartCost": 0,
      "RunCost": 0.45,
      "TotalCost": 0.45,
      "CLEstFromStat": false,
      "Input": {
        "Pages": 25,
        "Records": 25000,
        "RecordSize": 43
      },
      "Filter": {
        "MthSelectivity": 1
      },
      "Output": {
        "Records": 25000,
        "RecordSize": 43,
        "Sorted": false
      }
    },
    "Run": {
      "ContextID": 63123,
      "StartTimestamp": "2017-12-11-16.18.00.789831",
      "QueryTimeSpent": 0.040438,
      "GetMores": 25,
      "ReturnNum": 25000,
      "ReadRecords": 25000
    }
  }
}
```
