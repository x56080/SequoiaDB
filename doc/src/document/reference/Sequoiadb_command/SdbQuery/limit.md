##语法##
***query.limit( \<num\> )***

控制查询返回的记录条数。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| num |	int | 自定义返回结果集的记录条数。 | 否 |

> **Note：**  
> query.limit() 方法的定义格式包含 num 参数，它是 int 类型。如果不设定 num 的内容，相当于返回所有的结果集记录。如果想返回结果集的前5条记录，可是设置 num 的值为5。

##返回值##

返回结果集的游标，类型为 object 。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

选择集合 bar 下 age 字段值大于10的记录（如使用 [$gt](reference/operator/match_operator/gt.md) 查询），并只返回前面2条记录。

```lang-javascript
> db.foo.bar.find( { age: { $gt: 10 } } ).limit( 2 )
{
  "_id": {
    "$oid": "5813035cc842af52b6000009"
  },
  "name": "Tom",
  "age": 11
}
{
  "_id": {
    "$oid": "58130372c842af52b600000a"
  },
  "name": "Jack",
  "age": 12
}
```

> **Note：**  
> 如果结果集的记录数小于2，按实际的记录数返回，如果结果集的记录数大于2，则只返回前2条记录。
