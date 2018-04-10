##语法##

```
{ <字段名>: { $elemMatch: { <子字段名>: <表达式>, ... } } }
```

##描述##

选择集合中“<字段名>”匹配指定“{ <子字段>: <表达式> ... }”的记录。

其中“<表达式>”可以是值，也可以是带有[匹配符](reference/operator/match_operator/overview.md)的表达式，“$elemMatch”匹配符支持多层嵌套。

##示例##

在集合 foo.bar 中插入 3 条记录：

```lang-javascript
> db.foo.bar.insert( { "id": 1, "content": { "name": "Jack", "phone": "123", "address": "1000 Market Street, Philadelphia" } } )
> db.foo.bar.insert( { "id": 2, "content": [ { "name": "Tom", "phone": "456", "address": "2000 Market Street, Philadelphia" } ] } )
> db.foo.bar.insert( { "id": 3, "content": { "name": "Septem", "phone": "789", "address": { "addr1": "3000 Market Street, Philadelphia", "addr2": "4000 Market Street, Philadelphia" } } } )
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
{
  "_id": {
    "$oid": "5aaa25ee48bcff191e000002"
  },
  "id": 3,
  "content": {
    "name": "Septem",
    "phone": "789",
    "address": {
      "addr1": "3000 Market Street, Philadelphia",
      "addr2": "4000 Market Street, Philadelphia"
    }
  }
}
Return 3 row(s).
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

* 匹配“content”字段中子字段“phone”符合表达式“ $lte："123" ”的记录：

  ```lang-javascript
  > db.foo.bar.find( { content: { $elemMatch: { "phone" : { $lte : "123" } } } } )
  {
    "_id": {
      "$oid": "5a0106e51f9b983f4600000b"
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

* 使用“$regex”匹配符以及嵌套的“$elemMatch”匹配符：

  ```lang-javascript
  > db.foo.bar.find( { "content" : { $elemMatch : { address : { $elemMatch: { addr1 : { $regex : ".*Philadelphia$" } } } } } } )
  {
    "_id": {
      "$oid": "5a0107641f9b983f4600000d"
    },
    "id": 3,
    "content": {
      "name": "Septem",
      "phone": "789",
      "address": {
        "addr1": "3000 Market Street, Philadelphia",
        "addr2": "4000 Market Street, Philadelphia"
      }
    }
  }
  Return 1 row(s).
  ```