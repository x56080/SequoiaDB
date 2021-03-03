##语法##

```lang-json
{ <字段名>: { $ltrim: 1 } }
```

##说明##

去掉字符串左侧开头的空格( 包括回车符 '\\r'、换行符 '\\n' 以及制表符 '\\t' )。原始值为数组类型时对每个数组元素执行该操作，非字符串类型返回 null 。

##示例##

在集合 foo.bar 插入1条记录：

```lang-javascript 
> db.foo.bar.insert( { "a": " abc" } )
```

SequoiaDB shell 运行如下：

* 作为选择符使用，返回字段“a”去掉左侧开头空格之后的结果：

  ```lang-javascript
  > db.foo.bar.find( {}, { "a": { "$ltrim": 1 } } )
  {
      "_id": {
        "$oid": "582564592b4c38286d00001e"
      },
      "a": "abc"
  }
  Return 1 row(s).
  ```

  > **Note:**  
  > { $ltrim: 1 } 中的1作为占位符出现。

* 与匹配符配合使用，匹配字段“a”去掉左侧开头空格之后的值为"abc"的记录：
  
  ```lang-javascript
  > db.foo.bar.find( { "a": { "$ltrim": 1, "$et": "abc"} } )
  {
      "_id": {
        "$oid": "582564592b4c38286d00001e"
      },
      "a": " abc"
  }
  Return 1 row(s).
  ```
