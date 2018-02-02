##语法##

```
{ <字段名1>: { $elemMatchOne: <值1> }, <字段名2>: { $elemMatchOne: <值2>, ... } }
```

##描述##

返回数组内满足条件的第一个元素的集合。

##示例##

在集合 foo.bar 插入1条记录：

```lang-javascript

> db.foo.bar.insert( { "_id": 1, "class": 1, "students": [ { "name": "ZhangSan", "age": 18 }, { "name": "LiSi", "age": 19 }, { "name": "WangErmazi", "age": 18 } ] } )
```

SequoiaDB shell 运行如下：

* 查询集合 foo.bar 的记录，指定返回“age”等于18的第一个数组元素：

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
  Return 1 row(s).
  ```