##语法##

```lang-json
{ $pull: { <字段名1>: <值1>, <字段名2>: <值2>, ... } }
```

##描述##

$pull 要求操作的记录中 <字段名> 的值必须是数组，它从数组中删除与 <值> 相同的元素。

其中“<值>”支持以下几种格式：

* 任意类型的常量值，如：

  ```lang-json
  { $pull: { arr: 1 } }
  ```
  
  若 <值> 为对象时，需要数组元素的每个字段值都与 <值> 中的字段值相同，才认为匹配成功，并删除数组中的元素。
  
* 通过[字段操作符](reference/operator/field_operator/field.md)指定的原始记录中的某字段，如：

  ```lang-json
  { $pull: { arr: { $field: "fieldName" } } }
  ```
  
  如果 "fieldName" 字段在原始记录中不存在，则不做任何操作。

##示例##

* 操作 arr 字段，删除数组中为 2 的元素；操作 name 字段，删除数组中为 "Tom" 的元素。如有记录：

 ```lang-json
 { arr: [ 1, 2, 4, 5 ], age: 10, name: [ "Tom", "Mike" ] }
 ```

 ```lang-javascript
 > db.foo.bar.update( { $pull: { arr: 2, name: "Tom" } } )
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [ 1, 4, 5 ], age: 10, name: [ "Mike" ] }
 ```

* 操作 arr 字段，删除数组中为 2 的元素；操作 name 字段，删除数组中为 "Tom" 的元素。如有记录：

 ```lang-json
 { arr: [ 1, 3, 4, 5 ], age: 10, name: [ "Tom", "Mike" ] }
 ```

 ```lang-javascript
 > db.foo.bar.update( { $pull: { arr: 2, name: "Tom" } } )
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [ 1, 3, 4, 5 ], age: 10, name: [ "Mike" ] }
 ```

 由于 arr 数组没有元素值为 2 的元素，因此对 arr 字段不做任何操作。

* 操作 a 字段，删除数组中为 { id: 2 } 的元素。如有记录：

 ```lang-json
 { a: [ { id: 1, num: 1 }, { id: 2, num: 2 }, { id: 3, num: 3 }, { id: 4, num: 4 }, { id: 2 } ] }
 ```

 ```lang-javascript
 > db.foo.bar.update( { $pull: { a: { id: 2 } } } )
 ```

 此操作后，记录更新为：

 ```lang-json
 { a: [ { id: 1, num: 1 }, { id: 2, num: 2 }, { id: 3, num: 3 }, { id: 4, num: 4 } ] }
 ```

 由于要求字段全部匹配，因此只命中了第五个元素，未命中第二个元素。
 
* 使用其它字段的值从数组中删除元素。如有记录：

 ```lang-json
 { "arr": [ 1, 2, 20 ], "name": [ "Tom" ], age: 20 }
 ```

 使用 age 字段的值从 arr 字段中删除对应的值：
 
 ```lang-javascript
 > db.foo.bar.update({ $pull: { arr: { $field: "age" } } })
 ```

 此操作后，记录更新为：
 
 ```lang-json
 { "arr": [ 1, 2 ], "name": [ "Tom" ], age: 20 }
 ```
 