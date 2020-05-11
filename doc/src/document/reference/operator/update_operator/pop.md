##语法##

```lang-json
{ $pop: { <字段名1>: <值1>, <字段名2>: <值2>, ... } }
```

##描述##

$pop 操作是删除指定数组对象（<字段名1>,<字段名2>,...）最后 N 个元素，N 的大小由“<值>"决定。如果记录中不存在指定的数组对象，则不做任何操作；如果 N 大于数组对象的长度，数组对象的长度更新为 0，即它的元素全部被删除；如果 N < 0，则从数组起始删除 -N 个元素。

其中“<值>”支持以下两种格式：

* 数值常量，如：

  ```lang-json
  { $pop: { arr: 2 } }
  ```
  
* 通过 "$field" 指定的原始记录中某数值类型的字段，如：

  ```lang-json
  { $pop: { arr: { $field: "fieldName" } } }
  ```
  
  如果 "fieldName" 字段在原始记录中不存在，则不做任何操作；如果存在但不是数值类型，则报错。

##示例##

* 删除集合 bar 下数组对象 arr 的最后两个元素。如有记录：

 ```lang-json
 { arr: [1,2,3,4], age: 20, name: "Tom" }
 ```

 ```lang-javascript
 > db.foo.bar.update({ $pop: { arr: 2 } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [1,2], age:20, name: "Tom" }
 ```

* 删除集合bar下数组对象arr的最后10个元素。如有记录：

 ```lang-json
 { arr: [1,2,3,4], age: 20, name: "Tom" }
 ```

 ```lang-javascript
 > db.foo.bar.update({ $pop: { arr: 10 } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [], age: 20, name: "Tom" }
 ```

* 删除集合bar下数组对象arr的前两个元素，即设置N的值为-2。如有记录：

 ```lang-json
 { arr: [1,2,3,4], age: 20, name: "Tom" }
 ```

 ```lang-javascript
 > db.foo.bar.update({ $pop: { arr: -2 } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [3,4], age: 20, name: "Tom" }
 ```

* 在以上记录中新增一个字段 m，其值为 2，然后使用该字段的值对 arr 字段执行 $pop 操作：

 ```lang-javascript
 > db.foo.bar.update({ $set: { m: 2 } })
 > db.foo.bar.update({ $pop: { arr: { $field: "m" } } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [], age: 20, m: 2, name: "Tom" }
 ```
