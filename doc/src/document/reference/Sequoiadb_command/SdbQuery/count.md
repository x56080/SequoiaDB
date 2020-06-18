##语法##
***query.count()***

查询符合匹配条件的记录条数。

##返回值##

返回符合匹配条件的记录条数，类型为 object 。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

选择集合 bar 下 age 大于10的记录，使用 [$gt](reference/operator/match_operator/gt.md) 查询返回符合匹配条件 { age: { $gt: 10 } } 的记录条数。

```lang-javascript
> db.foo.bar.find( { age: { $gt: 10 } } ).count()
```

> **Note:**  
> query.count() 返回的结果忽略 [query.skip()](reference/Sequoiadb_command/SdbQuery/skip.md) 及 [query.limit()](reference/Sequoiadb_command/SdbQuery/limit.md) 的影响。