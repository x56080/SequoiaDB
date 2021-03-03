##语法##

```
{ <字段名>: { $gt: <值> } }
```

##描述##

$gt 选择满足“<字段名>”的值大于（\>）指定“<值>”的记录。

##示例##

* 返回集合 foo.bar 中“age”字段值大于20的记录。

  ```lang-javascript
  > db.foo.bar.find( { age: { $gt: 20 } } )
  ```

* $gt 匹配嵌套对象中的字段名。使用 update() 方法更新嵌套对象“service”中的“ID”字段值大于2的记录，将这些记录的“age”字段值设定为25。

  ```lang-javascript
  > db.foo.bar.update( { $set: { age: 25 } }, { "service.ID": { $gt: 2 } } )
  ```
