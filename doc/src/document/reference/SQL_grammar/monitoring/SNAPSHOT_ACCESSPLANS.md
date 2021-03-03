##描述##

访问计划缓存快照 $SNAPSHOT_ACCESSPLANS 列出数据库中缓存的访问计划的信息。

每一个数据节点上每个缓存的访问计划为一条记录。

##标示##

$SNAPSHOT_ACCESSPLANS

###字段信息###

| 字段名               | 类型      | 描述                                           |
| -------------------- | --------- | ---------------------------------------------- |
| NodeName             | 字符串    | 该访问计划所在的节点名（"主机名:端口号"）      |
| GroupName            | 字符串    | 该访问计划所在的数据组名                       |
| AccessPlanID         | 长整型    | 该访问计划的 ID 标识                           |
| Collection           | 字符串    | 该访问计划所在的集合名                         |
| CollectionSpace      | 字符串    | 该访问计划所在的集合空间名                     |
| ScanType             | 字符串    | 该访问计划的扫描类型：<br/>1. "tbscan"为表扫描<br/>2. "ixscan"为索引扫描 |
| IndexName            | 字符串    | 该访问计划使用的索引，表扫描时该项为空字符串"" |
| UseExtSort           | 布尔型    | 该访问计划是否需要对扫描结果进行排序           |
| CacheLevel           | 字符串    | 该访问计划的缓存级别：<br/>1. "OPT_PLAN_NOCACHE"为不进行缓存<br/>2. "OPT_PLAN_ORIGINAL"为缓存原访问计划<br/>3. "OPT_PLAN_NORMALZIED"为缓存泛化后的访问计划<br/>4. "OPT_PLAN_PARAMETERIZED"为缓存参数化的访问计划<br/>5. "OPT_PLAN_FUZZYOPTR"为缓存参数化并带操作符模糊匹配的访问计划 |
| Query                | BSON 对象 | 该访问计划的匹配符                             |
| Sort                 | BSON 对象 | 该访问计划的排序字段                           |
| Hint                 | BSON 对象 | 该访问计划指定使用索引的提示                   |
| SortedIndexRequired  | 布尔型    | 该访问计划是否需要使用根据 Sort 排序的索引     |
| EstFromStat          | 布尔型    | 该访问计划是否使用统计信息生成                 |
| HashCode             | 整型      | 该访问计划的 hash 值                           |
| Score                | 浮点型    | 该访问计划的评分                               |
| RefCount             | 整型      | 该访问计划正在被使用的查询个数                 |
| ParamPlanValid       | 布尔型    | 该访问计划是否参数化有效的访问计划             |
| MainCLPlanValid      | 布尔型    | 该访问计划是否主表有效的访问计划               |
| ValidParams          | BSON 对象数组 | 该访问计划已经生效的参数列表（ParamPlanValid 为 false 时显示） |
| ValidParams.Parameters | 数组   | 该访问计划生效的参数                            |
| ValidParams.Score      | 浮点数 | 该访问计划生效的参数的评分                      |
| ValidSubCLs          | BSON 对象数组 | 该访问计划已经生效的参数列表（MainCLPlanValid 为 false 时显示） |
| ValidSubCLs.Collection | 字符串 | 该访问计划为主表访问计划时生效的子表名          |
| ValidSubCLs.Parameters | 数组   | 该访问计划生效的参数                            |
| ValidSubCLs.Score      | 浮点数 | 该访问计划生效的参数的评分                      |
| AccessCount          | 长整型    | 该访问计划使用次数累计                         |
| TotalQueryTime       | 浮点型    | 该访问计划的累计执行时间（单位：秒）           |
| AvgQueryTime         | 浮点型    | 该访问计划的平均执行时间（单位：秒）           |
| MaxTimeSpentQuery    | BSON 对象 | 该访问计划执行时间最慢的一次查询信息           |
| MinTimeSpentQuery    | BSON 对象 | 该访问计划执行时间最快的一次查询信息           |

### MaxTimeSpentQuery 和 MinTimeSpentQuery 对象的信息 ###

| 字段名               | 类型      | 描述                                           |
| -------------------- | --------- | ---------------------------------------------- |
| ContextID            | 长整型    | 查询所在的上下文 ID                            |
| QueryType            | 字符串    | 查询的类型：<br/>1. "SELECT"：数据查询<br/>2. "UPDATE"：数据更新<br/>3. "DELETE"：数据删除 |
| QueryTimeSpent       | 浮点型    | 查询执行的时间（单位：秒）                     |
| ExecuteTimeSpent     | 浮点型    | 额外操作（UPDATE 或者 DELETE）执行的时间（单位：秒） |
| Parameters           | 数组      | 查询使用的参数                                 |
| StartTimestamp       | 字符串    | 查询启动的时间戳                               |
| Selector             | BSON 对象 | 查询使用的选择符                               |
| Skip                 | 长整型    | 查询需要跳过的记录个数                         |
| Return               | 长整型    | 查询需要最多返回的记录个数                     |
| Flag                 | 整型      | 查询使用的标志位                               |
| DataRead             | 长整型    | 查询读取的记录个数                             |
| IndexRead            | 长整型    | 查询通过索引读取的记录个数                     |
| GetMores             | 整型      | 查询返回的结果集次数                           |
| ReturnNum            | 长整型    | 查询返回的记录个数                             |
| HitEnd               | 布尔型    | 查询是否返回所有需要的结果                     |

>   **Note:**
>
>   *   最慢和最快的查询 MaxTimeSpentQuery 和 MinTimeSpentQuery 只计算查询的 QueryTimeSpent，不计算查询的 ExecuteTimeSpent。
>   *   SdbQuery.explain() 的 Run 模式不计算在 QueryTimeSpent 、MaxTimeSpentQuery 和 MinTimeSpentQuery 中。

##示例##

```lang-javascript
> db.exec( "select * from $SNAPSHOT_ACCESSPLANS" )
{
  "NodeName": "hostname:30000",
  "GroupName": "SYSCatalogGroup",
  "AccessPlanID": 14,
  "Collection": "SYSCAT.SYSTASKS",
  "CollectionSpace": "SYSCAT",
  "ScanType": "tbscan",
  "IndexName": "",
  "UseExtSort": false,
  "CacheLevel": "OPT_PLAN_ORIGINAL",
  "Query": {
    "TaskType": 0,
    "Name": "local_test_cs.local_test_sub2",
    "Target": "db1",
    "Source": "db2",
    "SplitValue": {
      "b": 8000
    }
  },
  "Sort": {},
  "Hint": {},
  "SortedIndexRequired": false,
  "EstFromStat": false,
  "HashCode": 377049080,
  "Score": 3.125000000000001e-12,
  "RefCount": 1,
  "AccessCount": 1,
  "TotalQueryTime": 0,
  "AvgQueryTime": 0,
  "MaxTimeSpentQuery": {
    "ContextID": 76605,
    "QueryType": "SELECT",
    "StartTimestamp": "2019-05-31-10.28.45.584948",
    "QueryTimeSpent": 0,
    "ExecuteTimeSpent": 0,
    "Selector": {},
    "Skip": 0,
    "Return": -1,
    "Flag": 0,
    "DataRead": 1,
    "IndexRead": 0,
    "GetMores": 1,
    "ReturnNum": 1,
    "HitEnd": true
  },
  "MinTimeSpentQuery": {
    "ContextID": 76605,
    "QueryType": "SELECT",
    "StartTimestamp": "2019-05-31-10.28.45.584948",
    "QueryTimeSpent": 0,
    "ExecuteTimeSpent": 0,
    "Selector": {},
    "Skip": 0,
    "Return": -1,
    "Flag": 0,
    "DataRead": 1,
    "IndexRead": 0,
    "GetMores": 1,
    "ReturnNum": 1,
    "HitEnd": true
  }
}
...
```
