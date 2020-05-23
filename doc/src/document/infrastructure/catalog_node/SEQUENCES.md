##所属集合空间##

SYSGTS

##概念##

SYSGTS.SEQUENCES 集合中包含了该集群中所有的自增字段信息。每个自增字段保存为一个文档。

每个文档包含以下字段：

|  字段名      |   类型  |    描述                                     |
|--------------|---------|------------------------------------------   |
| Name         | String  | 自增字段名                                  |
| AcquireSize  | Int32   | 协调节点每次获取的序列值的数量，可参考 [AcquireSize](data_model/auto_increment.md)                            |
| CacheSize    | Int32   | 编目节点每次缓存的序列值的数量，取值须大于0 |
| CurrentValue | Int64   | 自增字段的当前值，可参考 [CurrentValue](data_model/auto_increment.md)                                         |
| Cycled       | Bool    | 序列值达到最大值或最小值时是否允许循环 <br> "true"：允许循环 <br> "false"：不允许循环                              | 
| ID           | Int64   | 自增字段 ID
| Increment    | Int32   | 自增字段每次增加的间隔，可参考 [Increment](data_model/auto_increment.md)                                         |
| Initial      | Bool    | 序列是否已经分配过序列值 <br> "true"：未分配过序列值 <br> "false"：已分配过序列值                                      |
| Internal     | Bool    | 自增字段由系统内部定义还是由用户定义 <br> "true"：系统内部定义 <br> "false"：用户定义（由用户定义的自增字段暂未开放）                             |
| MaxValue     | Int64   | 自增字段的最大值，可参考 [MaxValue](data_model/auto_increment.md)                                         |
| MinValue     | Int64   | 自增字段的最小值                            |
| StartValue   | Int64   | 自增字段的起始值                            |
| Version      | Int64   | 自增字段的版本号                            |

##示例##

一个典型的自增字段信息如下：

```lang-json
> var db = new Sdb("localhost",11810)
> db.SYSGTS.SEQUENCES.find()
{
  "AcquireSize": 1000,
  "CacheSize": 1000,
  "CurrentValue": 1001,
  "Cycled": false,
  "ID": 3,
  "Increment": 1,
  "Initial": false,
  "Internal": true,
  "MaxValue": {
    "$numberLong": "9223372036854775807"
  },
  "MinValue": 1,
  "Name": "SYS_4294967303_studentID_SEQ",
  "StartValue": 1,
  "Version": 0,
  "_id": {
    "$oid": "5ea7e6bbd200b5897ef049ce"
  }
}
```


