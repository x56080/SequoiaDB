##语法##

$match与[db.collectionspace.collection.find()](reference/Sequoiadb_command/SdbCollection/find.md)方法中的cond参数完全相同。

##描述##

通过$match可以从集合中选择匹配条件的记录。

##示例##

- 下面的示例使用$match执行简单的匹配:

 ```lang-javascript
 > db.foo.bar.aggregate({ $match: { $and: [ { score: 80 }, { "info.name": { $exists: 1 } } ] } })
 ```

 该操作表示从集合foo.bar中返回符合条件score等于80且info对象中的子对象name字段存在的记录。

- 下面的示例使用$match匹配符合条件的记录，然后使用$group对结果集分组，最后使用$project输出结果集中指定的字段名:

 ```lang-javascript
 > db.foo.bar.aggregate({ $match: { $and: [ { score: 80 }, { "info.name": { $exists: 1 } } ] } }, { $group: { _id: "$major" } }, { $project: { major: 1, dep: 1 } })
 ```

 该操作首先集合foo.bar中返回符合条件score等于80且info对象中的子对象name字段存在的记录，然后按 major字段进行分组，最后选择输出结果集中的major和dep字段。
