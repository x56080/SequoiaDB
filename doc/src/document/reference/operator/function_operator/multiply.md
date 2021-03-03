##语法##

```
{ <字段名>: { $multiply: <值> } }
```

##说明##

返回字段值与某个数值相乘的结果。原始值为数组类型时对每个数组元素执行该操作，非数字类型返回 null 。

##示例##

在集合 foo.bar 插入1条记录：

```lang-javascript 
> db.foo.bar.insert( { "a": 3 } )
```

SequoiaDB shell 运行如下：

* 作为选择符使用，返回字段“a”乘以10的结果：

  ```lang-javascript
  > db.foo.bar.find( {}, { "a": { "$multiply": 10 } } )
  {
      "_id": {
        "$oid": "58256a7b2b4c38286d000020"
      },
      "a": 30
  }
  Return 1 row(s).
  ```

* 与匹配符配合使用，匹配字段“a”乘以10之积为30的记录：
  
  ```lang-javascript
  > db.foo.bar.find( { "a": { "$multiply": 10, "$et": 30 } } )
  {
      "_id": {
        "$oid": "58256a7b2b4c38286d000020"
      },
      "a": 3
  }
  Return 1 row(s).
  ```