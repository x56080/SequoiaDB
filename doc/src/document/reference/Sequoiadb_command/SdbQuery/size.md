##语法##
***query.size()***

返回当前游标到最终游标的记录条数。

##返回值##

返回记录条数，类型为 number。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

选择集合 bar 下 age 大于10的记录（如使用 [$gt](reference/operator/match_operator/gt.md) 查询），返回当前游标到最终游标的记录条数。

```lang-javascript
> db.foo.bar.find( { age: { $gt: 10 } } ).size()
```

> **Note：**  
> query.size()返回的结果考虑 query.skip() 及 query.limit() 的影响。