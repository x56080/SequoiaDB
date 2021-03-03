##概念##

每一个分区集合都会默认创建一个名叫“$shard”的索引，该索引叫做分区索引。

非分区集合不存在分区索引。

分区索引存在于分区集合所在的每一个分区组中，其字段定义顺序和排列与分区键相同。

>**Note:**
>
>任何用户定义的唯一索引必须包含分区索引中所有的字段，其字段顺序无关。
>
>在分区集合中，_id 字段仅保证分区内该字段唯一，无法保证全局唯一。

##示例##

一个典型的分区索引如下：

```lang-javascript
> db.foo.bar.listIndexes()
{
  "IndexDef": 
  {
    "name": "$shard",
    "_id": { "$oid": "515954bfa88873112fa6bd3a" },
    "key": { "Field1": 1, "Field2": -1 },
    "v": 0,
    "unique": false,
    "dropDups": false,
    "enforced": false
  },
  "IndexFlag": "Normal"
}
```
