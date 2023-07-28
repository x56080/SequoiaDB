##语法##

```lang-json
{ $saveMax: { <字段名1>: <值1>, <字段名2>: <值2>, ... } }
```

##描述##

$saveMax 操作是将指定字段的值更新为指定的值，当且仅当给定的值大于字段的值才发生更新。如果原记录中没有指定的字段名，那将字段名和值填充到记录中。
如果指定的值与指定字段的值类型不同，会按照BSON比较顺序进行比较。


其中“<值>”支持以下几种格式：

* 任意类型的值，如：

  ```lang-json
  { $saveMax: { field: 2 } }
  ```

* 通过[字段操作符][field]指定的原始记录中的某字段，如：

  ```lang-json
  { $saveMax: { field: { $field: "fieldName" } } }
  ```

##示例##

* 集合 sample.employee 存在如下记录：

  ```lang-json
  { "a": 60, "b": 70, "c": "str" }
  ```

* 使用 $saveMax 指定值为 80 更新字段 a

  ```lang-javascript
  > db.sample.employee.update({ $saveMax: { a: 80 } })
  ```

  此操作后，由于指定的值大于原字段的值，发生了更新，记录更新为：

  ```lang-json
  { "a": 80, "b": 70, "c": "str" }
  ```

* 使用 $saveMax 指定值为 "aaa" 更新字段 a

  ```lang-javascript
  > db.sample.employee.update({ $saveMax: { a: "aaa" } })
  ```

  此操作后，由于指定的值的类型大小大于原字段的类型大小，发生了更新，记录更新为：

  ```lang-json
  { "a": "aaa", "b": 70, "c": "str" }
  ```

* 在 $saveMax 中使用一个字段c更新另一个字段a

  ```lang-javascript
  > db.sample.employee.update({ $saveMax: { a: { $field: "c" }}})
  ```

  此操作后, 记录更新为:
  ```lang-json
  { "a": "str", "b": 70, "c": "str" }
  ```

[^_^]:
     本文使用的引用及链接
[field]:manual/Manual/Operator/Field_Operator/field.md