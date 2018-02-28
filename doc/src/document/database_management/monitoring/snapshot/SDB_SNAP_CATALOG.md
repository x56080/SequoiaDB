##描述##

编目信息快照 SDB_SNAP_CATALOG 列出当前数据库中所有集合的编目信息，每个集合一条记录。

##标示##

SDB_SNAP_CATALOG

>   **Note:**
>
>   只能在协调节点执行。

##字段信息##

| 字段名              | 类型   | 描述                         |
| ------------------- | ------ | ---------------------------- |
| Name                | 字符串 | 集合完整名                   |
| EnsureShardingIndex | 布尔   | 是否自动为分区键字段创建索引 |
| ReplSize            | 整型   | 执行修改操作时需要同步的副本数<br>当执行更新、插入、删除记录等操作时，仅当指定副本数的节点都完成操作时才返回操作结果 |
| ShardingKey         | 对象   | 数据分区类型：<br>- range：数据按分区键值的范围进行分区存储<br>- hash：数据按分区键的哈希值进行分区存储 |
| Version             | 整型   | 集合版本号，当对集合的元数据执行修改操作时递增该版本号（例如数据切分） |
| Attribute           | 整形   | 集合属性                     |
| AttributeDesc       | 字符串 | 集合属性描述                 |
| CataInfo.GroupID    | 整型   | 分区组 ID                    |
| CataInfo.GroupName  | 字符串 | 分区组名                     |
| CataInfo.LowBound   | 对象   | 数据分区区间的上限           |
| CataInfo.UpBound    | 对象   | 数据分区区间的下限           |

##示例##

```lang-javascript
> db.snapshot( SDB_SNAP_CATALOG )
{
  "_id": {
    "$oid": "5247a2bc60080822db1cfba2"
  },
  "Name": "foo.bar",
  "Version": 1,
  "Attribute": 0,
  "AttributeDesc": "",
  "ReplSize": 1,
  "ShardingKey": {
    "age": 1
  },
  "EnsureShardingIndex": true,
  "ShardingType": "range",
  "CataInfo": [
    {
      "GroupID": 1000,
      "GroupName": "group1",
      "LowBound": {
        "": {
          "$minKey": 1
        }
      },
      "UpBound": {
        "": {
          "$maxKey": 1
        }
      }
    }
  ]
}
```
