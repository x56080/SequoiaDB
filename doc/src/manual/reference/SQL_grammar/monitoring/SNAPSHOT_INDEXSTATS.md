##描述##

[索引统计信息](manual/Maintainance/Access_Plan/statistics.md)快照 $SNAPSHOT_INDEXSTATS 列出当前数据库节点中所有的索引统计信息。每条索引为一条记录。

##标示##

$SNAPSHOT_INDEXSTATS

##字段信息##

| 字段名          | 数据类型 | 说明 |
| --------------- | -------- | ---- |
| NodeName        | 字符串   | 集合所属节点名（主机名：端口号）|
| GroupName       | 字符串   | 集合所属分区组名 |
| Collection      | 字符串   | 统计的集合名称 |
| StatTimestamp   | 字符串   | 统计收集的时间 |
| Index           | 字符串   | 统计Index的名称 |
| TotalIndexLevels| 整型     | 统计收集时索引的层数 |
| TotalIndexPages | 整型     | 统计收集时索引的页个数 |
| Unique          | 布尔     | Index是否唯一索引 |
| KeyPattern      | 对象     | 统计索引的字段定义，例如：{a:1, b:-1} |
| DistinctValNum  | 数组     | 不重复的值的个数。抽样时，指样本中不重复值的个数。<br>数组第 1 个元素表示字段定义中第 1 个字段的不重复值个数；第 2 个元素表示字段定义中第 1 和第 2 个字段的不重复值个数。以此类推。<br>例如，字段定义为`{a:1, b:-1}`，数组为 [50, 100]，则 a 字段的不重复值有 50 个，a 和 b 字段组合的不重复值有 100 个。|
| MinValue        | 对象     | 索引最小值。抽样时，指样本中的最小值 |
| MaxValue        | 对象     | 索引最大值。抽样时，指样本中的最大值 |
| NullFrac        | 整型     | null 值的万分比。抽样时，指样本中 null 值的万分比 |
| UndefFrac       | 整型     | undefined 值的万分比。抽样时，指样本中 undefined 值的万分比 |
| SampleRecords   | 长整型   | 统计收集时抽样的文档个数 |
| TotalRecords    | 长整型   | 统计收集时的文档个数 |

##示例##

```lang-javascript
> db.exec( "select * from $SNAPSHOT_INDEXSTATS" )
{
  "NodeName": "hostname:11840",
  "GroupName": "group2",
  "Collection": "sample.employees",
  "StatTimestamp": "2020-06-19-14.10.38.931000",
  "Index": "index01",
  "TotalIndexLevels": 2,
  "TotalIndexPages": 135,
  "Unique": false,
  "KeyPattern": {
    "activityType": 1,
    "status": 1
  },
  "DistinctValNum": [
    2,
    8
  ],
  "MinValue": {
    "activityType": 1,
    "status": 1
  },
  "MaxValue": {
    "activityType": 2,
    "status": 9
  },
  "NullFrac": 0,
  "UndefFrac": 0,
  "SampleRecords": 200,
  "TotalRecords": 136276
}
...
```
