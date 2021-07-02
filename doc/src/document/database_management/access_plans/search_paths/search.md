使用 [SdbQuery.explain\(\)](reference/Sequoiadb_command/SdbQuery/explain.md) 可以查看访问计划的搜索过程的信息。

当 SdbQuery.explain() 的 Search 选项为 true 时，将展示以下信息（Evaluate 选项为 true 时显示 Constants 和 Input 字段）：

| 字段名                         | 类型      | 描述                                                                                 |
| ------------------------------ | --------- | ------------------------------------------------------------------------------------ |
| Constants                      | BSON 对象 | 生成访问计划使用的常量<br>Evaluate 选项为 true 时显示                                |
| Constants.RandomReadIOCostUnit | 整型      | 随机读取 IO 的代价，默认值为 10                                                      |
| Constants.SeqReadIOCostUnit    | 整型      | 顺序读取 IO 的代价，默认值为 1                                                       |
| Constants.SeqWrtIOCostUnit     | 整型      | 顺序写入 IO 的代价，默认值为 2                                                       |
| Constants.PageUnit             | 整型      | 数据页的单位，默认值为 4096 （单位：字节）                                           |
| Constants.RecExtractCPUCost    | 整型      | 从数据页中提取数据的 CPU 代价，默认值为 4                                            |
| Constants.IXExtractCPUCost     | 整型      | 从索引页中提取索引项的 CPU 代价，默认值为 2                                          |
| Constants.OptrCPUCost          | 整型      | 操作符的 CPU 代价单位，默认值为 1                                                    |
| Constants.IOCPURate            | 整型      | IO 代价与 CPU 代价的比例，默认值为 2000                                              |
| Constants.TBScanStartCost      | 整型      | 全表扫描的启动代价，默认值为 0                                                       |
| Constants.IXScanStartCost      | 整型      | 索引扫描的启动代价，默认值为 0                                                       |
| Options                        | BSON      | 生成访问计划使用的配置项，即 SequoiaDB 的配置                                        |
| Options.optcostthreshold       | 整型      | SequoiaDB 的 --optcostthreshold 选项，查询优化器忽略 IO 影响的最小的页数<br>数据页数大于阈值时，估算访问计划代价时需要计算 IO 的代价<br>默认值为 20，0 表示一直需要计算 IO 代价，-1 表示从不计算代价 |
| Options.sortbuf                | 整型      | SequoiaDB 的 --sortbuf 选项<br>排序缓存大小（单位：MB），默认值为 256 ，最小值为 128 |
| Input                          | BSON 对象 | 生成访问计划使用的输入项，集合的统计信息<br>Evaluate 选项为 true 时显示              |
| Input.Pages                    | 整型      | 集合的数据页个数                                                                     |
| Input.Records                  | 长整型    | 集合的数据个数                                                                       |
| Input.RecordSize               | 整型      | 集合的数据平均长度                                                                   |
| Input.NeedEvalIO               | 布尔型    | 根据 Input.Pages 和 Options.optcostthreshold 判断是否需要计算 IO 代价                |
| Input.CLEstFromStat            | 布尔型    | 是否使用集合的统计信息进行估算                                                       |
| Input.CLStatTime               | 时间戳    | 使用的集合的统计信息的生成时间                                                       |
| SearchPaths                    | 数组      | 每个搜索过的访问计划的估算过程                                                       |

>   **Note:**
>
>   *   Constants 字段下的值为常量不可进行设置
>   *   Options 字段下的值可以通过 SequoiaDB 的配置来设置

SearchPaths 数组的每项表示一个搜索过的访问计划，将展示以下信息：

| 字段名         | 类型      | 描述                                             |
| -------------- | --------- | ------------------------------------------------ |
| ScanType       | 字符串    | 访问计划的扫描方式<br>1. "tbscan" 表示全表扫描<br>2. "ixscan" 表示索引扫描 |
| IndexName      | 字符串    | 访问计划使用的索引的名称<br>全表扫描时，字段值为 ""      |
| UseExtSort     | 布尔型    | 访问计划是否使用非索引排序                       |
| Direction      | 整型      | 访问计划使用索引时的扫描方向<br>1 表示正向扫描索引<br>-1 表示反向扫描索引 |
| Query          | BSON 对象 | 访问计划解析后的用户查询条件                     |
| IXBound        | BSON 对象 | 访问计划使用索引的查找范围<br>全表扫描时，字段值为 null    |
| NeedMatch      | 布尔型    | 访问计划获取记录时是否需要根据匹配符进行过滤<br>NeedMatch 为 false 的情况有：<br>1. 没有查询条件<br>2. 查询条件可以被索引覆盖 |
| IXEstFromStat  | 布尔型    | 是否使用索引的统计信息进行估算（索引扫描时显示） |
| IXStatTime     | 时间戳    | 使用的索引的统计信息的生成时间（索引扫描时显示） |
| Score          | 浮点型    | 评分：<br>1. 索引扫描为索引的选择率（< 0.1时为候选计划）<br>2. 全表扫描为匹配符的选择率 |
| IsCandidate    | 布尔型    | 是否候选访问计划，不是候选计划不进行估算<br>1. 索引扫描选择率 < 0.1<br>2. 索引扫描完全匹配排序字段<br>3. 全表扫描 |
| IsUsed         | 布尔型    | 是否最终选择的访问计划                           |
| TotalCost      | 浮点型    | 估算的代价（内部表示 单位约为 1/2000000 秒）<br>该代价不包括选择符、skip() 和 limit() 的影响 |
| ScanNode       | BSON 对象 | [TBSCAN 的推演公式](database_management/access_plans/search_paths/TBSCAN.md) 或 [IXSCAN 推演公式](database_management/access_plans/search_paths/IXSCAN.md)<br>Evaluate 选项为 true 时显示 |
| SortNode       | BSON 对象 | [SORT 的推演公式](database_management/access_plans/search_paths/SORT.md)<br>Evaluate 选项为 true 且需要进行排序时显示 |

Evaluate 选项为 true 时将展示查询优化器的推演公式，每个需要计算的变量将以数组形式展示：

```lang-json
变量: [
  公式,
  代入数据的计算公式,
  计算结果
]
```

*   [TBSCAN的推演公式](database_management/access_plans/search_paths/TBSCAN.md)
*   [IXSCAN的推演公式](database_management/access_plans/search_paths/IXSCAN.md)
*   [SORT的推演公式](database_management/access_plans/search_paths/SORT.md)

>   **Note:**
>
>   推演公式中的代价均为内部表示，单位约为 1/2000000 秒


**示例**

```lang-json
{
  ...,
  "Search": {
    "Options": {
      "sortbuf": 256,
      "optcostthreshold": 20
    },
    "Constants": {
      "RandomReadIOCostUnit": 10,
      "SeqReadIOCostUnit": 1,
      "SeqWrtIOCostUnit": 2,
      "PageUnit": 4096,
      "RecExtractCPUCost": 4,
      "IXExtractCPUCost": 2,
      "OptrCPUCost": 1,
      "IOCPURate": 2000,
      "TBScanStartCost": 0,
      "IXScanStartCost": 0
    },
    "Input": {
      "Pages": 1,
      "Records": 10,
      "NeedEvalIO": false,
      "CLEstFromStat": false
    },
    "SearchPaths": [
      {
        "IsUsed": false,
        "IsCandidate": false,
        "Score": 1,
        "ScanType": "ixscan",
        "IndexName": "$id",
        "UseExtSort": false,
        "Direction": 1,
        "IXBound": {
          "_id": [
            [
              {
                "$minElement": 1
              },
              {
                "$maxElement": 1
              }
            ]
          ]
        },
        "NeedMatch": false,
        "IXEstFromStat": false
      },
      ...
    ]
  }
}
```
