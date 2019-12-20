##语法##
***db.collectionspace.collection.aggregate\( \<subOp1\>，[subOp2]，... \)***

aggregate() 方法与 [find()](reference/Sequoiadb_command/SdbCollection/find.md)方法功能比较接近，也是从 SequoiaDB 的集合中检索文档记录，并返回游标。

##参数描述##

| 参数名 | 参数类型  | 描述   | 是否必填 |
| ------ | ------    | ------ | ------   |
| subOp1,subOp2...  | json 对象 | 表示包含[聚集符] (reference/operator/aggregate_operator/overview.md)的子操作，在 aggregate() 方法中可以填写 1~N 个子操作。| 是 |

> **Note:**
>
> * aggregate() 方法可以有任意多个子操作，每个子操作是一个包含聚集符的 JSON 对象，子操作之间用逗号隔开。注意各子操作的参数名的语法规则。

##返回值##

返回游标。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

假设集合 collection 包含如下格式的记录：

```
{
  no:1000,
  score:80,
  interest:["basketball","football"],
  major:"计算机科学与技术",
  dep:"计算机学院",
  info:
  {
    name:"Tom",
    age:25,
    gender:"男"
  }
}
```

* 按条件选择记录，并指定返回字段名，如下聚集操作操作首先使用 $match 选择匹配条件的记录，然后使用 $project 只返回指定的字段名。

 ```lang-javascript
  > db.foo.bar.aggregate( { $match: { $and: [ { no: { $gt: 1002 } },{ no: { $lt: 1015 } },{ dep: "计算机学院" } ] } },{ $project: { no: 1, "info.name": 1, major: 1 } } )
 {
      "no": 1003,
      "info.name": "Sam",
      "major": "计算机软件与理论"
 }
 {
      "no": 1004,
      "info.name": "Coll",
      "major": "计算机工程"
 }
 {
      "no": 1005,
      "info.name": "Jim",
      "major": "计算机工程"
 }
 ```

* 按条件选择记录，并对记录进行分组。如下操作首先使用 $match 选择匹配条件的记录，然后使用 $group 对记录按字段 major 进行分组，并使用 $avg 返回每个分组中嵌套对象 age 字段的平均值。

 ```lang-javascript
 > db.foo.bar.aggregate( { $match: { dep:  "计算机学院" } },{ $group: { _id:  "$major", Major: { $first: "$major" }, avg_age: { $avg: "$info.age" } } } ) 
 {
      "Major": "计算机工程",
      "avg_age": 25
 }
 {
      "Major": "计算机科学与技术",
      "avg_age": 22.5
 }
 {
      "Major": "计算机软件与理论",
      "avg_age": 26
 }
 ```

* 按条件选择记录，并对记录进行分组、排序、限制返回记录的起始位置和返回记录数,。如下操作首先按 $match 选择匹配条件的记录；然后使用 $group 按 major 进行分组，并使用 $avg 返回每个分组中嵌套对象 age 字段的平均值，输出字段名为 avg_age； 最后使用 $sort 按 avg_age 字段值（降序），major 字段值（降序）对结果集进行排序，使用 $skip 确定返回记录的起始位置，使用 $limit 限制返回记录的条数。

 ```lang-javascript
 > db.foo.bar.aggregate( { $match: { interest: { $exists: 1 } } }, { $group: { _id: "$major", avg_age: { $avg: "$info.age" }, major: { $first: "$major" } } }, { $sort: { avg_age: -1, major: -1 } }, { $skip: 2 }, { $limit: 3 } )
 {
      "avg_age": 25,
      "major": "计算机科学与技术"
 }
 {
      "avg_age": 22,
      "major": "计算机软件与理论"
 }
 {
      "avg_age": 22,
      "major": "物理学"
 }
 ```
