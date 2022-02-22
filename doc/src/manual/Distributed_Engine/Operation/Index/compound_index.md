[^_^]:
     复合索引

SequoiaDB 巨杉数据库支持创建复合索引。复合索引是指结合记录中多个字段创建的索引。如果用户在查询时经常使用某几个字段，可以为这些字段创建复合索引。查询操作使用复合索引时，会比任意一个单字段索引的查询速度更快，使查询更加高效。

##使用##

集合 sample.employee 中存在如下记录：

```lang-json
{"area_id": 1, "name": "Mark", "score": 432, "info": {"age": 18, "city": "Shanghai"}}
{"area_id": 1, "name": "Adam", "score": 475, "info": {"age": 32, "city": "Shanghai"}}
{"area_id": 2, "name": "Sam", "score": 494, "info": {"age": 28, "city": "Guangzhou"}}
{"area_id": 2, "name": "Jenny", "score": 483, "info": {"age": 23, "city": "Guangzhou"}}
```

以集合的 name 和 info 字段创建正序索引，索引名为“sortIdx”

```lang-javascript
> db.sample.employee.createIndex("sortIdx", {"name": 1, "info": 1})
```

> **Note:**
>
> 创建索引的详细参数说明可参考 [createIndex()][createIndex]。

查看条件为{name: "Sam", "info.age": 28}的查询所对应的访问计划

```lang-javascript
> db.sample.employee.find({name: "Sam", "info.age": 28}).explain()
```

输出信息中显示该查询已使用索引 sortIdx

```lang-json
{
  ···
  "Role": "data",
  "Name": "sample.employee",
  "ScanType": "ixscan",
  "IndexName": "sortIdx",
  ···
}
```

##前缀##

复合索引支持最左前缀原则，以实现索引的复用，减少数据库因维护索引而导致的资源消耗。例如，集合中存在索引 testIdx，字段定义如下：

```lang-javascript
> db.sample.employee.createIndex("testIdx", {"x": 1, "y": 1, "z": 1})
```

那么该索引具有以下前缀：

```lang-json
{"x": 1}
{"x": 1, "y": 1}
```

因此以下查询均可以使用该索引：

```lang-javascript
> db.sample.employee.find({"x": 10, "y": 10, "z": 100})
> db.sample.employee.find({"x": 10, "y": 10})
> db.sample.employee.find({"x": 10, "z": 10})
> db.sample.employee.find({"x": 10})
```

而类似 `{"y": 10}`、`{"y": 10, "z": 100}` 的查询条件则无法使用该索引。

##参考##

更多操作可参考

| 操作        | 说明               |
| ----------- | ------------------ |
| [query.explain()][explain] | 获取查询的访问计划 |
| [SdbCollection.dropIndex()][dropIndex] | 删除指定索引 |




[^_^]:
    本文使用的所有引用及链接
[createIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
[access_plan]:manual/Distributed_Engine/Maintainance/Access_Plan/Readme.md
[hint]:manual/Manual/Sequoiadb_Command/SdbQuery/hint.md
[explain]:manual/Manual/Sequoiadb_Command/SdbQuery/explain.md
[dropIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/dropIndex.md