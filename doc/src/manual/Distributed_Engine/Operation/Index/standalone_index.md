[^_^]:
     独立索引

SequoiaDB 巨杉数据库支持创建独立索引。独立索引是指在集合的部分数据节点上单独创建的索引。如果用户希望只在指定的数据节点中使用索引，可以为这些节点创建独立索引。

独立索引在编目节点上没有元数据信息，且索引的相关操作不写入同步日志中，因此增量同步、全量同步和数据切分时将忽略该索引。

##使用##

分区集合 sample.employee 中存在如下记录：

```lang-json
{"id": 1, "name": "Mark", "score": 432, "info": {"age": 18, "city": "Shanghai"}}
{"id": 2, "name": "Adam", "score": 475, "info": {"age": 32, "city": "Beijing"}}
{"id": 3, "name": "Sam", "score": 494, "info": {"age": 28, "city": "Guangzhou"}}
{"id": 4, "name": "Jenny", "score": 483, "info": {"age": 23, "city": "Guangzhou"}}
```

在节点 `sdbserver:11820` 上创建独立索引，索引名为“ageIdx”

```lang-javascript
> db.sample.employee.createIndex("ageIdx", {"info.age": 1 }, {Standalone: true}, {NodeName: "sdbserver:11820"})
```

> **Note:**
>
> 创建索引的详细参数说明可参考 [createIndex()][createIndex]。

查看条件为{"info.age": 28}的查询所对应的访问计划

```lang-javascript
> db.sample.employee.find({"info.age": 28}).explain()
```

输出信息中显示该查询仅在节点 11820 上使用索引 ageIdx，其余节点上的查询均为全表扫描

```lang-json
{
  "NodeName": "sdbserver:11820",
  "GroupName": "group1",
  "Role": "data",
  "Name": "sample.employee",
  "ScanType": "ixscan",
  "IndexName": "ageIdx",
  ···
}
{
  "NodeName": "sdbserver:11830",
  "GroupName": "group2",
  "Role": "data",
  "Name": "sample.employee",
  "ScanType": "tbscan",
  "IndexName": "",
  ···
}
{
  "NodeName": "sdbserver:11840",
  "GroupName": "group3",
  "Role": "data",
  "Name": "sample.employee",
  "ScanType": "tbscan",
  "IndexName": "",
  ···
}
```

##参考##

更多操作可参考

| 操作        | 说明               |
| ----------- | ------------------ |
| [query.explain()][explain] | 获取查询的访问计划 |
| [SdbCollection.dropIndex()][dropIndex] | 删除指定索引 |






[^_^]:
    本文使用的所有引用及链接
[createIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
[hint]:manual/Manual/Sequoiadb_Command/SdbQuery/hint.md
[access_plan]:manual/Distributed_Engine/Maintainance/Access_Plan/Readme.md
[explain]:manual/Manual/Sequoiadb_Command/SdbQuery/explain.md
[dropIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/dropIndex.md