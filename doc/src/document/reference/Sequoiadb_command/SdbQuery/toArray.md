##语法##
***query.toArray()***

以数组的形式返回结果集。

##返回值##

返回结果集为数组。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

以数组的形式返回集合 bar 中 age 字段值大于5的记录（如使用 [$gt](reference/operator/match_operator/gt.md) 查询）。

```lang-javascript
> db.foo.bar.find( { age: { $gt: 10 } } ).toArray()
{
  "_id": {
    "$oid": "516a76a1c9565daf06030000"
  },
  "age": 10,
  "name": "Tom"
},{
  "_id": {
    "$oid": "516a76a1c9565daf06050000"
  },
  "age": 20,
  "a": 10
},{
  "_id": {
    "$oid": "516a76a1c9565daf06040000"
  },
  "age": 15
}
```
