[^_^]:
    唯一索引

SequoiaDB 巨杉数据库支持创建唯一索引，用于保证索引字段值的唯一性。在使用唯一索引时，如果插入或更新的索引字段值在集合中已存在，则操作报错。

##使用##

集合 sample.employee 存在如下记录：

```lang-json
{"id": 1, "name": "Mark", "score": 432, "info": {"age": 18, "city": "Shanghai"}}
{"id": 2, "name": "Adam", "score": 475, "info": {"age": 32, "city": "Beijing"}}
```

以集合的 id 字段创建唯一索引，索引名为“uniqueIdx”

```lang-javascript
> db.sample.employee.createIndex("uniqueIdx", {"id": 1}, {"Unique": true})
```

> **Note:**
>
> 创建索引的详细参数说明可参考 [createIndex()][createIndex]。

当插入的 id 值在集合中已存在时，操作将报错

```lang-javascript
> db.sample.employee.insert({"id": 2, "name": "Sam", "score": 494, "info": {"age": 28, "city": "Guangzhou"}})
sdb.js:646 uncaught exception: -38
Duplicate key exist
```

##注意事项##

用户在使用唯一索引时，存在以下注意事项：

- 默认情况下，唯一索引允许存在多个全空的索引键。
- 唯一索引包含多个字段时，仅在每个字段均相同的情况下，才认为索引字段值重复。
- 在分区集合中创建唯一索引时，索引建需包含所有分区键。

##参考##

更多操作可参考

| 操作        | 说明               |
| ----------- | ------------------ |
| [query.explain()][explain] | 获取查询的访问计划 |
| [SdbCollection.dropIndex()][dropIndex] | 删除指定索引 |




[^_^]:
    本文使用的所有引用及链接
[createIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
[explain]:manual/Manual/Sequoiadb_Command/SdbQuery/explain.md
[dropIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/dropIndex.md


