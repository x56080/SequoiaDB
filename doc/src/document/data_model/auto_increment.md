##概念##

自增字段是集合中可以自动生成唯一序列的字段。集合允许拥有多个自增字段。

为保证高性能，自增字段的序列值是在编目节点中统一生成，并批量分配给协调节点的。因此默认地，自增字段的值只保证趋势递增（或递减），但不保证连续分配。以递增序列为例，如果多个协调节点同时插入数据，在小的区间内，可能会出现后插入的文档的序列值比先插入的小，但在大的区间内，数值是递增的。如需连续递增，也可以设置有关属性来实现。

##属性##

| 属性名                   | 类型     | 必填 | 默认值    | 描述 | 
| ------------------------ | -------- | -----| --------- | ---- |
| Field                    | String   | 是   | -         | 自增字段名，必须是可见字符，不能以“$”或空白字符起始 |
| Increment                | Int32    | 否   | 1         | 自增字段每次增加的间隔，可以为正整数或负整数，不能为0 |
| StartValue               | Int64    | 否   | 1         | 自增字段的起始值 |
| CurrentValue             | Int64    | -    | -         | 自增字段的当前值，可以在[序列快照](database_management/monitoring/snapshot/SDB_SNAP_SEQUENCES.md)中查看。不能创建时指定 |
| MinValue                 | Int64    | 否   | 1         | 自增字段的最小值 |
| MaxValue                 | Int64    | 否   | 2^63 -1   | 自增字段的最大值 |
| CacheSize                | Int32    | 否   | 1000      | 编目节点每次缓存的序列值的数量，取值须大于0 |
| AcquireSize              | Int32    | 否   | 1000      | 协调节点每次获取的序列值的数量，取值须大于0，小于等于CacheSize |
| Cycled                   | Bool     | 否   | false     | 序列值达到最大值或最小值时是否允许循环 |
| Generated                | String   | 否   | "default" | 自增字段生成方式，取值为"always", "default"或"strict"。<br>"always" 表示自增字段总是由服务端生成，忽略客户端的设置；<br>"default" 表示缺省时生成，允许客户端的设置；<br>"strict" 则在允许客户端设置的同时增加类型检测，类型不为数值时报错。 |

>   **Note:**
>
>   * 自增字段可以是嵌套字段。
>   * Increment为正整数时，StartValue默认为1，MinValue默认为1，MaxValue默认为2^63 -1。<br>Increment为负整数时，StartValue默认为-1，MinValue默认为-2^63 ，MaxValue默认为-1。
>   * StartValue必须位于[MinValue,MaxValue]区间。
>   * 当设置AcquireSize为1时，可以实现序列连续递增（或递减）。CacheSize和AcquireSize是性能相关的参数，建议谨慎修改。
>   * 使用主子表时，仅主表自增字段会生效，子表自增字段无效。
>   * 如客户端设置过自增字段的值，或使用中修改过自增字段的属性，字段值将可能不唯一。如需保证修改后值唯一，建议使用唯一索引。
>   * 独立节点不支持自增字段。

##SequoiaDB Shell支持的操作##

+ **创建**

    1、可以使用 [SdbCS.createCL\(\)](reference/Sequoiadb_command/SdbCS/createCL.md) 接口创建集合时，在options指定AutoIncrement参数来创建。<br>
    2、在已有的集合上使用 [SdbCollection.createAutoIncrement\(\)](reference/Sequoiadb_command/SdbCollection/createAutoIncrement.md) 来创建。

+ **删除**

    可通过接口 [SdbCollection.dropAutoIncrement\(\)](reference/Sequoiadb_command/SdbCollection/dropAutoIncrement.md) 实现删除。

+ **修改**

    可通过接口 [SdbCollection.setAttributes\(\)](reference/Sequoiadb_command/SdbCollection/setAttributes.md)  在options指定AutoIncrement参数修改自增字段的属性。

+ **查看**

    在集合的 [编目信息快照](database_management/monitoring/snapshot/SDB_SNAP_CATALOG.md) 的AutoIncrement字段中，可以查看现有的自增字段。如要查看详细的自增字段属性信息，还需查找对应的 [序列快照](database_management/monitoring/snapshot/SDB_SNAP_SEQUENCES.md)。

##示例##

1. 在创建集合时指定自增字段为studentID。

    ```lang-javascript
    > db.foo.createCL("bar", { AutoIncrement: { Field: "studentID" } })
    localhost:11810.foo.bar
    Takes 0.586001s.
    > db.foo.bar.insert({ name: "Tom" })
    Takes 0.033371s.
    > db.foo.bar.find()
    {
      "_id": {
        "$oid": "5bd7da4260ff5166c507aa22"
      },
      "name": "Tom",
      "studentID": 1
    }
    Return 1 row(s).
    Takes 0.006712s.
    ```

2. 在集合上创建起始值为5000自增字段，并修改增加间隔为10。然后查看集合自增字段属性。

    ```lang-javascript
    > db.foo.bar.createAutoIncrement({ Field: "studentID", StartValue: 5000 })
    Takes 0.069783s.
    > db.foo.bar.setAttributes({ AutoIncrement: { Field: "studentID", Increment: 10 } })
    Takes 0.039292s.
    >
    > db.snapshot( SDB_SNAP_CATALOG, { Name: "foo.bar" }, { AutoIncrement: 1 } )
    {
      "AutoIncrement": [
        {
          "SequenceName": "SYS_21333102559237_studentID_SEQ",
          "Field": "studentID",
          "Generated": "default",
          "SequenceID": 4
        }
      ]
    }
    Return 1 row(s).
    Takes 0.006737s.
    > 
    > db.snapshot( SDB_SNAP_SEQUENCES, { Name: "SYS_21333102559237_studentID_SEQ" } )
    {
      "AcquireSize": 1000,
      "CacheSize": 1000,
      "CurrentValue": 5000,
      "Cycled": false,
      "ID": 4,
      "Increment": 10,
      "Initial": true,
      "Internal": true,
      "MaxValue": {
        "$numberLong": "9223372036854775807"
      },
      "MinValue": 1,
      "Name": "SYS_21333102559237_studentID_SEQ",
      "StartValue": 5000,
      "Version": 1,
      "_id": {
        "$oid": "5bd8fcfc8af29ca6ad2a32e8"
      }
    }
    Return 1 row(s).
    Takes 0.012240s.
    ```

3. 在集合上创建一个嵌套和非嵌套的自增字段，然后删除非嵌套的字段。

    ```lang-javascript
    > db.foo.bar.createAutoIncrement([ { "Field": "No" }, { Field: "student.ID" } ])
    Takes 0.628813s.
    > db.foo.bar.insert({ student: { "name": "Tom" } })
    Takes 0.006884s.
    > db.foo.bar.find()
    {
      "_id": {
        "$oid": "5bd7e2478c74f9c1a14366ca"
      },
      "student": {
        "name": "Tom",
        "ID": 1
      },
      "No": 1
    }
    Return 1 row(s).
    Takes 0.007183s.
    > db.foo.bar.dropAutoIncrement("No")
    Takes 0.066828s.
    > db.foo.bar.insert({ student: { "name": "Jerry" } })
    Takes 0.003347s.
    > db.foo.bar.find()
    {
      "_id": {
        "$oid": "5bd7e2478c74f9c1a14366ca"
      },
      "student": {
        "name": "Tom",
        "ID": 1
      },
      "No": 1
    }
    {
      "_id": {
        "$oid": "5bd7e2b08c74f9c1a14366cb"
      },
      "student": {
        "name": "Jerry",
        "ID": 2
      }
    }
    Return 2 row(s).
    Takes 0.010535s.
    ```
