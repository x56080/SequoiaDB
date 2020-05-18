##语法##

```lang-json
{ $field: <字段名> }
```
  
##描述##
$field 是取字段符，用于取出指定字段的值，应用于其它操作，支持的操作包括：

- 用于查询操作的匹配条件中，语法格式如下：

  ```lang-json
  { <字段名1>: { $field: <字段名2> }, ... }
  { <字段名1>: { <匹配符>: { $field: <字段名2> } }, ... }
  ```

- 用于更新操作的更新规则中，支持的更新符包括 [$inc](reference/operator/update_operator/inc.md)、[$set](reference/operator/update_operator/set)、[$pop](reference/operator/update_operator/pop)、[$pull](reference/operator/update_operator/pull)、[$pull_by](reference/operator/update_operator/pull_by) 及 [$push](reference/operator/update_operator/push)，语法格式如下：

  ```lang-json
  { <更新符>: { <字段名1>: { $field: <字段名2> } } }
  ```

  对于 $inc 操作，由于其值的部分还支持对象的形式，因此，还支持如下语法格式：
 
  ```lang-json
  { $inc: { <字段名1>: { Value: { $field: <字段名2> } [, Default: 默认值, Min: 最小值, Max: 最大值 ] } } }
  ``` 

##示例##

此处只举若干示例，更多示例请到各操作符章节查看。

- 创建集合 company.employee 并插入以下 2 条记录：

  ```lang-json
  { "t1": 100, "t2": 100 } 
  { "t1": 200, "t2": 150 }
  ```

- 查询 company.employee 中 “t1” 字段值等于 “t2” 字段的记录：

  ```lang-javascript
  > db.company.employee.find( { "t1": { "$field": "t2" } } )
  {
      "t1": 100,
      "t2": 100
  }
  ```

- 查询 company.employee 中 “t1” 字段值大于 “t2” 字段的记录：

  ```lang-javascript
  > db.company.employee.find( { "t1": { "$gt": { "$field": "t2" } } } )
  {
      "t1": 200,
      "t2": 150
  }
  ```
  
- 将字段 "t2" 设置为与 "t1" 相同：

  ```lang-javascript
  > db.company.employee.update({ $set: { t2: { $field: "t1" } } })
  ```
 
  该操作完成后，记录如下：
  
  ```lang-json
  { "t1": 100, "t2": 100 }
  { "t1": 200, "t2": 200 } 
  ```
 
- 将字段 "t2" 的值累加到 "t1" 上：

  ```lang-javascript
  > db.company.employee.update({ $inc: { t1: { Value: { $field: "t2" } } } })
  ```
 
  该操作完成后，记录如下：
 
  ```lang-json
  { "t1": 200, "t2": 100 }
  { "t1": 400, "t2": 200 } 
  ```
 
  
