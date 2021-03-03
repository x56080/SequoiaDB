##语法##

```
{ <字段名>: { $ne: <值> } }
```

##描述##

$ne 选择满足“<字段名>”的值不等于（!=）指定“<值>”的记录。

##示例##

* 查询集合 foo.bar 中“age”字段值不等于 20 的记录：

  ```lang-javascript
  > db.foo.bar.find( { "age": { "$ne": 20 } } )
  ```

* $ne 匹配嵌套对象中的字段名。使用 update() 方法更新嵌套对象“service”中的“type”字段值不等于15的记录，将这些记录的“age”字段值设定为25：

  ```lang-javascript
  > db.foo.bar.update( { "$set": { "age": 25 } }, { "service.type": { "$ne": 15 } } )
  ```
