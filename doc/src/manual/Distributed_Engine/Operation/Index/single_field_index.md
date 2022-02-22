[^_^]:
    单字段索引

SequoiaDB 巨杉数据库支持创建单字段索引。单字段索引是指仅根据记录中某一字段创建的索引。

##以单个字段创建索引##

集合 sample.employee 中存在如下记录：

```lang-json
{"id": 1, "name": "Mark", "score": 432, "info": {"age": 18, "city": "Shanghai"}}
{"id": 2, "name": "Adam", "score": 475, "info": {"age": 32, "city": "Beijing"}}
{"id": 3, "name": "Sam", "score": 494, "info": {"age": 28, "city": "Guangzhou"}}
{"id": 4, "name": "Jenny", "score": 483, "info": {"age": 23, "city": "Guangzhou"}}
```

以集合的 score 字段创建倒序索引，索引名为“scoreIdx”

```lang-javascript
> db.sample.employee.createIndex("scoreIdx", {"score": -1})
```

> **Note:**
>
> 创建索引的详细参数说明可参考 [createIndex()][createIndex]。

查看条件为{"score": 28}的查询所对应的访问计划

```lang-javascript
> db.sample.employee.find({"score": 28}).explain()
```

输出信息中显示该查询已使用索引 scoreIdx

```lang-json
{
  ···
  "Role": "data",
  "Name": "sample.employee",
  "ScanType": "ixscan",
  "IndexName": "scoreIdx",
  ···
}
```

##以对象字段创建索引##

用户可以在集合中以对象字段创建索引，以快速查找对象字段中的数据。

集合 sample.employee 中存在如下记录：

```lang-json
{"id": 1, "name": "Mark", "score": 432, "info": {"age": 18, "city": "Shanghai"}}
{"id": 2, "name": "Adam", "score": 475, "info": {"age": 32, "city": "Beijing"}}
{"id": 3, "name": "Sam", "score": 494, "info": {"age": 28, "city": "Guangzhou"}}
{"id": 4, "name": "Jenny", "score": 483, "info": {"age": 23, "city": "Guangzhou"}}
```

以集合的 info 字段创建升序索引，索引名为“infoIdx”

```lang-javascript
> db.sample.employee.createIndex("infoIdx", {"info": 1})
```

查看条件为{"info": {"age": 32, "city": "Beijing"}}的查询所对应的访问计划

```lang-javascript
> db.sample.employee.find({"info": {"age": 32, "city": "Beijing"}}).explain()
```

输出信息中显示该查询已使用索引 infoIdx

```lang-json
{
  ···
  "Role": "data",
  "Name": "sample.employee",
  "ScanType": "ixscan",
  "IndexName": "infoIdx",
  ···
}
```

##以对象嵌套字段创建索引##

用户可以在集合中以对象嵌套字段创建索引，以实现更精准的查询。

集合 sample.employee 中存在如下记录：

```lang-json
{"id": 1, "name": "Mark", "score": 432, "info": {"age": 18, "city": "Shanghai"}}
{"id": 2, "name": "Adam", "score": 475, "info": {"age": 32, "city": "Beijing"}}
{"id": 3, "name": "Sam", "score": 494, "info": {"age": 28, "city": "Guangzhou"}}
{"id": 4, "name": "Jenny", "score": 483, "info": {"age": 23, "city": "Guangzhou"}}
```

以集合的 info.age 字段创建升序索引，索引名为“ageIdx”

```lang-javascript
> db.sample.employee.createIndex("ageIdx", {"info.age": 1})
```

查看条件为{"info.age": {$et: 28}}的查询所对应的访问计划

```lang-javascript
> db.sample.employee.find({"info.age": {$et: 28}}).explain()
```

输出信息中显示该查询已使用索引 ageIdx

```lang-json
{
  ···
  "Role": "data",
  "Name": "sample.employee",
  "ScanType": "ixscan",
  "IndexName": "ageIdx",
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
[access_plan]:manual/Distributed_Engine/Maintainance/Access_Plan/Readme.md
[hint]:manual/Manual/Sequoiadb_Command/SdbQuery/hint.md
[explain]:manual/Manual/Sequoiadb_Command/SdbQuery/explain.md
[dropIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/dropIndex.md