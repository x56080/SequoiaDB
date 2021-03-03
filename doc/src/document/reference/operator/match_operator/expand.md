##语法##

```
{ <字段名>: { $expand: 1 } }
```

> **Note:**  
> { $expand: 1 } 中1没有特殊含义，仅作为占位符出现。

##描述##

$expand 将数组中的元素展开，生成多条记录返回。字段不是数组类型时，则忽略该操作。

##示例##

在集合 foo.bar 插入1条记录：

```lang-javascript
> db.foo.bar.insert( { "a": [ 1,  { "b":1 } ], "c": 1 } )
```

SequoiaDB shell运行如下：

* 将集合 foo.bar 中 字段“a”展开并形成多条记录。

  ```lang-javascript
  > db.foo.bar.find( { "a": { "$expand": 1 } } )
  {
      "_id": {
        "$oid": "582439982b4c38286d000010"
      },
      "a": 1,
      "c": 1
  }
  {
      "_id": {
        "$oid": "582439982b4c38286d000010"
      },
      "a": {
        "b": 1
      },
      "c": 1
  }
  Return 2 row(s).
  ```
