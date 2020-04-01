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
| UniqueID            | 长整型 | 集合的UniqueID，在集群上全局唯一 |
| EnsureShardingIndex | 布尔   | 是否自动为分区键字段创建索引 |
| ReplSize            | 整型   | 执行修改操作时需要同步的副本数<br>当执行更新、插入、删除记录等操作时，仅当指定副本数的节点都完成操作时才返回操作结果 |
| ShardingKey         | 对象   | 数据分区类型：<br>- range：数据按分区键值的范围进行分区存储<br>- hash：数据按分区键的哈希值进行分区存储 |
| Version             | 整型   | 集合版本号，当对集合的元数据执行修改操作时递增该版本号（例如数据切分） |
| Attribute           | 整型   | 集合属性                     |
| AttributeDesc       | 字符串 | 集合属性描述                 |
| CompressionType     | 整型   | 压缩算法类型                 |
| CompressionTypeDesc | 字符串 | 压缩算法类型描述             |
| Partition           | 整型   | hash 分区的个数 ( 仅水平分区集合显示 )|
| InternalV           | 整型   | hash 算法版本号 ( 仅水平分区集合显示，内部使用 )      |
| AutoSplit           | 布尔   | 集合是否开启自动切分功能 ( 仅水平分区集合显示 )      |
| IsMainCL            | 布尔   | 集合是否为垂直分区中的主表 ( 仅垂直分区集合显示 )    |
| MainCLName          | 字符串 | 集合在垂直分区中所关联的主表名 ( 仅垂直分区集合显示 )|
| CataInfo.ID         | 整型   | 子表挂载的顺序 ID ( 内部使用 ) |
| CataInfo.SubCLName  | 字符串 | 子表名 ( 仅垂直分区集合显示 )  |
| CataInfo.GroupID    | 整型   | 分区组 ID                    |
| CataInfo.GroupName  | 字符串 | 分区组名                     |
| CataInfo.LowBound   | 对象   | 数据分区区间的上限           |
| CataInfo.UpBound    | 对象   | 数据分区区间的下限           |
| AutoIncrement.Field | 字符串 | 自增字段名称                 |
| AutoIncrement.Generated   | 字符串 | 自增字段生成方式       |
| AutoIncrement.SequenceName| 字符串 | 自增字段对应序列名     |
| AutoIncrement.SequenceID  | 长整型 | 自增字段对应序列ID     |

##示例##
1.普通集合

```lang-javascript
> db.snapshot( SDB_SNAP_CATALOG )
{
  "_id": {
    "$oid": "5e4245f9e86d05a0a03e69c8"
  },
  "Name": "sample.employee",
  "UniqueID": 4294967297,
  "Version": 1,
  "Attribute": 1,
  "AttributeDesc": "Compressed",
  "CompressionType": 1,
  "CompressionTypeDesc": "lzw",
  "CataInfo": [
    {
      "GroupID": 1000,
      "GroupName": "group1"
    }
  ]
}

```

2.水平分区集合

```lang-javascript
> db.snapshot( SDB_SNAP_CATALOG )
{
  "_id": {
    "$oid": "5247a2bc60080822db1cfba2"
  },
  "Name": "sample.employee",
  "UniqueID": 261993005057,
  "Version": 1,
  "Attribute": 0,
  "AttributeDesc": "",
  "AutoIncrement": [
    {
      "SequenceName": "SYS_261993005057_studentID_SEQ",
      "Field": "studentID",
      "Generated": "default",
      "SequenceID": 4
    }
  ],
  "CompressionType": 0,
  "CompressionTypeDesc": "snappy",
  "ReplSize": 1,
  "ShardingKey": {
    "age": 1
  },
  "EnsureShardingIndex": true,
  "ShardingType": "hash",
  "Partition": 4096,
  "InternalV": 3,
  "CataInfo": [
    {
      "ID": 0,
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
  "AutoSplit": ture,
}
```

3.垂直分区集合

```lang-javascript
> db.snapshot( SDB_SNAP_CATALOG )
{
  "_id": {
    "$oid": "5e426b88e86d05a0a03e69c9"
  }
  "Name": "year_2019.month",
  "UniqueID": 4294967298,
  "Attribute": 1,
  "AttributeDesc": "Compressed",
  "CataInfo": [
    {
      "ID": 1,
      "SubCLName": "year_2019.month_07",
      "LowBound": {
        "date": "20190701"
      },
      "UpBound": {
        "date": "20190801"
      }
    }
  ],
  "CompressionType": 1,
  "CompressionTypeDesc": "lzw",
  "EnsureShardingIndex": true,
  "IsMainCL": true,
  "LobShardingKeyFormat": "YYYYMMDD",
  "ShardingKey": {
    "date": 1
  },
  "ShardingType": "range",
  "Version": 2,
}

```