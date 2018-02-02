##语法##

```
{ <字段名1>: { $elemMatch: <值1> }, <字段名2>: { $elemMatch: <值2>, ... } }
```

##描述##

返回数组内满足条件的元素的集合。

##示例##

在集合 foo.bar 插入2条记录，一条是数组类型，一条是嵌套对象类型

```lang-javascript
> db.foo.bar.insert( { "_id": 1, "class": 1, "students": [ { "name": "ZhangSan", "age": 18 }, { "name": "LiSi", "age": 19 }, { "name": "WangErmazi", "age": 18 } ] } )
> db.foo.bar.insert( { "_id": 2, "class": 2, "students": { "name": "LinWu", "age": 18 } } )
> db.foo.bar.find()
{
  "_id": 1,
  "class": 1,
  "students": [
    {
      "name": "ZhangSan",
      "age": 18
    },
    {
      "name": "LiSi",
      "age": 19
    },
    {
      "name": "WangErmazi",
      "age": 18
    }
  ]
}
{
  "_id": 2,
  "class": 2,
  "students": {
    "name": "LinWu",
    "age": 18
  }
}
Return 2 row(s).
```

SequoiaDB shell 运行如下：

* 指定返回“age”等于18的数组元素：

  ```lang-javascript
  > db.foo.bar.find( {}, { "students": { "$elemMatch": { "age": 18 } } } )
    {
      "_id": 1,
      "class": 1,
      "students": [
        {
          "name": "ZhangSan",
          "age": 18
        },
        {
          "name": "WangErmazi",
          "age": 18
        }
      ]
    }
    {
      "_id": 2,
      "class": 2,
      "students": {
        "name": "LinWu",
        "age": 18
      }
    }
    Return 2 row(s).
  ```