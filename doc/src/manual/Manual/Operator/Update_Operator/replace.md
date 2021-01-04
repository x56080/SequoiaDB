
##语法##

```lang-json
{ $replace: { <字段名1>: <值1>, <字段名2>: <值2>, ... } }
```

##描述##

$replace 操作是将文档全部替换成{<字段名1>:<值1>,<字段名2>:<值2>,...}。除了保留原始的 _id 和自增字段之外，原始文档的内容会全部清空，并替换成{<字段名1>:<值1>,<字段名2>:<值2>,...}。

> **Note:**
> 
> 不支持保留嵌套的自增字段。

##示例##

选择集合 sample.employee 下不存在 age 字段的记录，使用 $replace 替换这些记录

```lang-javascript
> db.sample.employee.update({ $replace: { age: 0, name: 'default' } }, { age: { $exists: 0 } })
```
