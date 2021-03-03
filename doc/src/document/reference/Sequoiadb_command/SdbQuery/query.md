##语法##
***query[ i ]***

以下标的方式访问查询结果集。

##返回值##

返回指定记录，类型为 string 。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

```lang-javascript
> var query = db.foo.bar.find()
> println( query[0] )
{
  "_id": {
    "$oid": "58169cb3c842af52b600000c"
  },
  "a": 1
}
```
