
##语法##

```lang-json
{ $mul: { <字段名1>: <值1>, <字段名2>: <值2>, ... } }
```

##描述##

$mul 操作是给指定“<字段名>”乘以指定的“<值>”。如果原记录中没有指定的字段名，则将字段名和值填充到记录中，其值为0；如果原记录中存在指定的字段名，则将字段名的值乘以指定的值。

其中“<值>”支持以下几种格式：

- 数值类型

  ```lang-json
  { $mul: { salary: 1.1 } }
  ```
- 通过[字段操作符][field]指定的原始记录中某数值类型的字段

  ```lang-json
  { $mul: { salary: { $field: "fieldName" } } }
  ```

  >**Note:**
  >
  > 对于使用[字段操作符][field]的方式，如果 fieldName 字段在原始记录中不存在，或者虽然存在但不是数值类型，则不做任何操作。

- 对象类型，支持扩展功能，其中在 Value 字段的值中，也可以使用字段操作符

  ```lang-json
  { $mul: { salary: { Value: 1.1, Default:1, Min: 1, Max: 150} } }
  { $mul: { salary: { Value: { $field: "fieldName" }, Default:1, Min: 1, Max: 150} } }
  ```

  扩展功能说明：

  | 字段名 | 描述 |是否必填|
  | -------| -----|--------|
  | Value  | 指定需要乘以的值 <br> 只能为数值类型或通过 "$field" 指定的原始记录中数值类型的字段 |是|
  | Default| 当需要做mul操作的字段不存在时，取该值作为该字段的值 <br> 可以为数值类型或null，不填时默认值为 0；指定为 null 时表示当字段不存在时不做操作 |否|
  | Min    | 指定mul操作之后结果的最小值 <br> 只能为数值类型 |否|
  | Max    | 指定mul操作之后结果的最大值 <br> 只能为数值类型 |否|

##示例##

* 选择集合 employee 下 salary 字段值小于 3500 的记录，然后更新这些记录，将 salary 字段的值乘以 1.5。

 ```lang-javascript
 > db.sample.employee.update({ $mul: { salary: 1.5 } }, { salary: { $lt: 3500 } })
 ```

* 选择集合 sample.employee 下 salary 字段值小于 3500 的记录，使用字段 m 的值对 salary 字段进行 $mul 操作

 ```lang-javascript
 > db.sample.employee.update({ $mul: { salary: { $field: "m"} } }, { salary: { $lt: 3500 } })
 ```

* 选择集合 sample.employee 中存在数组对象 arr 的记录，将数组对象 arr 的第二个元素值乘以 2

 ```lang-javascript
 > db.sample.employee.update({ $mul: { "arr.1": 2 } }, { arr: { $exists: 1 } })
 ```

* 将集合 sample.employee 下 salary 字段的值乘以 5，并限定相乘后的结果必须在 0~10000 之间

 ```lang-javascript
 > db.sample.employee.update({ $mul: { salary: { Value: 5, Min: 0, Max: 10000 } } })
 ```

* 在集合 sample.employee 中，使用字段 m 的值对 salary 字段进行 $mul 操作，并限定相乘后的结果必须在 0~10000 之间

 ```lang-javascript
 > db.sample.employee.update({ $mul: { salary: { Value: { $field: "m" }, Min: 0, Max: 10000 } } })
 ```



[^_^]:
     本文使用的所有引用及链接
[field]:manual/Manual/Operator/Field_Operator/field.md
