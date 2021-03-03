##语法##

```
{ <字段名1>: { $field: <字段名2> }, ... }
```

或者

```
{ <字段名1>: { <匹配符>: { $field: <字段名2> } }, ... }
```

##描述##

$field 是字段符，选择满足“<字段名1>”匹配“<字段名2>”的记录。

##示例##

在集合 foo.bar 插入2条记录：

```lang-javascript
> db.foo.bar.insert( { "t1": 100, "t2": 100 } )
> db.foo.bar.insert( { "t1": 200, "t2": 150 } )
```

SequoiaDB shell 运行如下：

* 查询 foo.bar 中“t1”字段值等于“t2”字段的记录：

  ```lang-javascript
  > db.foo.bar.find( { "t1": { "$field": "t2" } } )
  {
      "_id": {
        "$oid": "5824313b2b4c38286d00000d"
      },
      "t1": 100,
      "t2": 100
  }
  Return 1 row(s).
  ```

* 查询 foo.bar 中“t1”字段值大于“t2”字段的记录：

  ```lang-javascript
  > db.foo.bar.find( { "t1": { "$gt": { "$field": "t2" } } } )
  {
      "_id": {
        "$oid": "582431452b4c38286d00000e"
      },
      "t1": 200,
      "t2": 150
  }
  Return 1 row(s).
  ```