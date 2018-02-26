##语法##

```
{ <字段名>: { $elemMatch: { <子字段名>: <值>, ... } } }
```

##描述##

选择集合中“<字段名>”匹配指定“{ <子字段>: <值> ... }”的记录。

##示例##

在集合 foo.bar 插入2条记录：

```lang-javascript
> db.foo.bar.insert( { "id": 1, "content": { "name": "Jack", "phone": "123", "address": "1000 Market Street, Philadelphia" } } )
> db.foo.bar.insert( { "id": 2, "content": [ { "name": "Tom", "phone": "456", "address": "2000 Market Street, Philadelphia" } ] } )
> db.foo.bar.find()
{
  "_id": {
    "$oid": "5a73ce416a3e18f64e000010"
  },
  "id": 1,
  "content": {
    "name": "Jack",
    "phone": "123",
    "address": "1000 Market Street, Philadelphia"
  }
}
{
  "_id": {
    "$oid": "5a73ce476a3e18f64e000011"
  },
  "id": 2,
  "content": [
    {
      "name": "Tom",
      "phone": "456",
      "address": "2000 Market Street, Philadelphia"
    }
  ]
}
Return 2 row(s).
```

SequoiaDB shell 运行如下：

* 嵌套对象匹配：

  ```lang-javascript
  > db.foo.bar.find( { "content": { $elemMatch: { "name": "Jack", "phone": "123" } } } )
  {
      "_id": {
        "$oid": "5822868a2b4c38286d000007"
      },
      "id": 1,
      "content": {
        "name": "Jack",
        "phone": "123",
        "address": "1000 Market Street, Philadelphia"
      }
  }
  Return 1 row(s).
  ```

* 数组匹配：

  ```lang-javascript
  > db.foo.bar.find( { "content": { $elemMatch: { "name": "Tom", "phone": "456" } } } )
  {
      "_id": {
        "$oid": "5822868a2b4c38286d000008"
      },
      "id": 2,
      "content": [
        {
          "name": "Tom",
          "phone": "456",
          "address": "2000 Market Street, Philadelphia"
        }
      ]
  }
  Return 1 row(s).
  ```