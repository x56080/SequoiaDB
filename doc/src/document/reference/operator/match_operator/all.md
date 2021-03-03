##语法##

```
{ <字段名>: { $all: [ <值1>, <值2>, ... ] } }
```

##描述##

$all 的操作对象是数组类型的字段名，选择“<字段名>”包含所有给定数组“[ <值1>, <值2>, ... ]”中的值。

##示例##

在集合 foo.bar 插入2条记录：

```lang-javascript
> db.foo.bar.insert( { "name": [ "Tom", "Mike", "Jack" ] } )

> db.foo.bar.insert( { "name": [ "Tom", "John" ], "age": 20 } )
```

SequoiaDB shell 运行如下：

* 查询集合 foo.bar 所有“name”字段的值包含“Tom”和“Mike”的记录。

  ```lang-javascript
  > db.foo.bar.find( { "name": { "$all": [ "Tom", "Mike" ] } } )
  {
      "_id": {
        "$oid": "582187282b4c38286d000001"
      },
      "name": [
        "Tom",
        "Mike",
        "Jack"
      ]
  }
  Return 1 row(s).
  ```
* 使用 $all 匹配非数组类型的字段，效果和 equal 一样：

  ```lang-javascript
  > db.foo.bar.find( { "age": { "$all": [ 20 ] } } )
  {
      "_id": {
        "$oid": "58218a132b4c38286d000002"
      },
      "name": [
        "Tom",
        "John"
      ],
      "age": 20
  }
  Return 1 row(s).

  > db.foo.bar.find( { "age": 20 } )
  {
      "_id": {
        "$oid": "58218a132b4c38286d000002"
      },
      "name": [
        "Tom",
        "John"
      ],
      "age": 20
  }
  Return 1 row(s).
  ```