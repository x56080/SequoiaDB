##语法##

```lang-json
{ <字段名>: { $add: <值> } }
```

##说明##

返回字段值加上某个数值的结果。原始值为数组类型时对每个数组元素执行该操作，非数字类型返回 null 。

##示例##

在集合 foo.bar 插入1条记录：

```lang-javascript 
> db.foo.bar.insert( { "a": 20 } )
```

SequoiaDB shell 运行如下：

* 作为选择符使用，字段“a”加上10的结果：

  ```lang-javascript
  > db.foo.bar.find( {}, { "a": { "$add": 10 } } )
  {
      "_id": {
        "$oid": "58251ca32b4c38286d000018"
      },
      "a": 30
  }
  Return 1 row(s).
  ```

* 与匹配符配合使用，字段“a”加10的和为30的记录：
  
  ```lang-javascript
  > db.foo.bar.find( { "a": { "$add": 10, "$et": 30 } } )
  {
      "_id": {
        "$oid": "58251ca32b4c38286d000018"
      },
      "a": 20
  }
  Return 1 row(s).
  ```