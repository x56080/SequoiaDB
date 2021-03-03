##语法##

```
{ <字段名1>: { $include: <0|非0> }, <字段名2>: { $include: <0|非0>, ... } }
```

##说明##

$include 可以指定选择或移除某个字段。

| 值  | 作用 |
| --- | ---- |
| 非0 | 选择字段 |
| 0   | 移除字段 |

##示例##

在集合 foo.bar 插入1条记录：

```lang-javascript

> db.foo.bar.insert( { "_id": 1, "class": 1, "students": [ { "name": "ZhangSan", "age": 18 }, { "name": "LiSi", "age": 19 }, { "name": "WangErmazi", "age": 18 } ] } )
```

SequoiaDB shell 运行如下：

* 查询集合 foo.bar，指定选择“students”字段：

  ```lang-javascript
  > db.foo.bar.find( {}, { "students": { "$include": 1 } } )
  {
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
  Return 1 row(s).
  ```

* 查询集合 foo.bar，指定移除“students”字段：

  ```lang-javascript
  > db.foo.bar.find( {}, { "students": { "$include": 0 } } )
  {
      "_id": 1,
      "class": 1
  }
  ```