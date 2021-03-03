##语法##
***query.explain( [option] )***

获取查询的访问计划。

##参数描述##

| 参数名 | 参数类型  | 描述 | 是否必填 |
| ------ | --------- | ---- | -------- |
| option | json 对象 | 访问计划执行参数，目前有 Run 字段项，表示是否执行访问计划，true 表示执行访问计划，获取数据和时间信息，false 表示只获取访问计划的信息，并不执行。默认为 false。 | 否 |

##访问计划描述##

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| Name | string | 集合名。 |
| ScanType | string | 扫描方式。表扫描：“tbscan”；索引扫描：“ixscan”。 |
| IndexName | string | 使用索引的名称。 |
| UseExtSort | bool | 是否使用非索引排序。 |
| Query | Json | 解析后的用户查询条件。 |
| IXBound | Json |  索引的查找范围。 |
| NeedMatch | bool | 获取记录时是否需要根据Query进行过滤，其中 NeedMatch 为 false 的情况有：<br>1. 没有查询条件。<br>2. 查询条件可以被索引覆盖。 |
| NodeName | string | 节点名。 |
| GroupName | string | 复制组名。 |
| ReturnNum | int64 | 返回记录的数量。 |
| ElapsedTime | float64 | 查询耗时（秒）。 |
| IndexRead | int64 | 索引记录扫描条数。 |
| DataRead | int64 | 数据记录扫描条数。 |
| UserCPU | float64 | 用户态 CPU 使用时间（秒）。 |
| SysCPU | float64 | 内核态 CPU 使用时间（秒）。 |
| SubCollections | Json Array | 垂直分区表中各子表的访问计划。 |

> **Note:**  
> 如果集合经过 split 分布在多个复制组，访问计划会按照一组一记录的方式返回。

##返回值##

返回访问计划的游标，类型为 object 。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

foo.bar 是一个水平分区集合，分布在两个分区组上。

```lang-javascript
> db.foo.bar.find({a:{$gt:1}}).explain( { Run: true } )
{
  "Name": "foo.bar",
  "ScanType": "ixscan",
  "IndexName": "$shard",
  "UseExtSort": false,
  "Query": {
    "$and": [
      {
        "a": {
          "$gt": 1
        }
      }
    ]
  },
  "IXBound": {
    "a": [
      [
        1,
        {
          "$maxElement": 1
        }
      ]
    ]
  },
  "NeedMatch": false,
  "NodeName": "hostname1:11830",
  "GroupName": "group1",
  "ReturnNum": 0,
  "ElapsedTime": 0.000107,
  "IndexRead": 0,
  "DataRead": 0,
  "UserCPU": 0,
  "SysCPU": 0
}
{
  "Name": "foo.bar",
  "ScanType": "ixscan",
  "IndexName": "$shard",
  "UseExtSort": false,
  "Query": {
    "$and": [
      {
        "a": {
          "$gt": 1
        }
      }
    ]
  },
  "IXBound": {
    "a": [
      [
        1,
        {
          "$maxElement": 1
        }
      ]
    ]
  },
  "NeedMatch": false,
  "NodeName": "hostname2:11840",
  "GroupName": "group2",
  "ReturnNum": 0,
  "ElapsedTime": 0.000104,
  "IndexRead": 0,
  "DataRead": 0,
  "UserCPU": 0,
  "SysCPU": 0
}
```
