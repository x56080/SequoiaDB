##语法##

```
{ <字段名1>: { $elemMatchOne: <表达式1> }, <字段名2>: { $elemMatchOne: <表达式2>, ... } }
```

##描述##

返回数组内满足条件的第一个元素的集合，或者嵌套对象中满足条件的子对象。

其中“<表达式>”可以是值，也可以是带有[匹配符](reference/operator/match_operator/overview.md)的表达式，“$elemMatch”匹配符支持多层嵌套。

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

* 指定返回“age”等于 18 的第一个数组元素：

  ```lang-javascript
  > db.foo.bar.find( {}, { "students": { "$elemMatchOne": { "age": 18 } } } )
    {
      "_id": 1,
      "class": 1,
      "students": [
        {
          "name": "ZhangSan",
          "age": 18
        }
      ]
    }
    {
      "_id": 2,
      "class": 2,
      "students": {
        "age": 18
      }
    }
    Return 2 row(s).
  ```

* 指定返回 class 1 中 “age”小于 19 的第一个学生，使用“$lt”表达式：

  ```lang-javascript
  > db.foo.bar.find( { class: 1}, { "students": { "$elemMatchOne": { "age": { $lt: 19 } } } } )
  {
      "_id": 1,
      "class": 1,
      "students": [
        {
          "name": "ZhangSan",
          "age": 18
        }
      ]
  }
  Return 1 row(s).
  ```