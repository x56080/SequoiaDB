##语法##

```
{ <字段名>: { $size: 1 } }
```

##说明##

返回数组或对象Object的元素个数。

##示例##

在集合 foo.bar 插入1条记录：

```lang-javascript 
> db.foo.bar.insert( { "a": [ 1, 2, 3 ] } )
```

SequoiaDB shell 运行如下：

* 作为选择符使用，返回字段“a”数组元素个数：

  ```lang-javascript
  > db.foo.bar.find( {}, { "a": { "$size": 1 } } )
  {
      "_id": {
        "$oid": "582575dd2b4c38286d000022"
      },
      "a": 3
  }
  Return 1 row(s).
  ```

  > **Note:**  
  > { $size: 1 } 中1没有特殊含义，仅作为占位符出现。

* 与匹配符配合使用，匹配字段“a”数组元素个数为3的记录：
  
  ```lang-javascript
  > db.foo.bar.find( { "a": { "$size": 1, "$et": 3 } } )
  {
      "_id": {
        "$oid": "582575dd2b4c38286d000022"
      },
      "a": [
        1,
        2,
        3
      ]
  }
  Return 1 row(s).
  ```
