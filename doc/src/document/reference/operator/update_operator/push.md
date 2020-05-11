##语法##

```lang-json
{ $push: {<字段名1>: <值1>, <字段名2>: <值2>, ... } }
```

##描述##

$push将给定数值（<值1>）插入到目标数组（<字段名1>）中，操作对象必须为数组类型的字段。如果记录中不存在指定的字段名，将指定的字段名以数组对象的形式推入到记录中并填充其指定的数值；如果记录中存在指定的字段名，且字段名存在指定的数值，指定的数值也会被推入到记录中。

其中“<值>”支持以下几种格式：

* 任意类型的常量值，如：

  ```lang-json
  { $push: { arr: 1 } }
  ```
  
* 通过 "$field" 指定的原始记录中的某字段，如：

  ```lang-json
  { $push: { arr: { $field: "fieldName" } } }
  ```
  
  如果 "fieldName" 字段在原始记录中不存在，则不做任何操作。

##示例##

* 向集合bar下的arr数组对象推入数值1。原记录中arr数组对象存在元素1，如有记录：

 ```lang-json
 { arr: [1,2,4], age: 10, name: ["Tom","Mike"] }
 ```

 ```lang-javascript
 > db.foo.bar.update({ $push: { arr: 1 } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [1,2,4,1], age: 10, name: ["Tom","Mike"] }
 ```

 虽然原来arr中有元素1，使用$push操作符，还是会将元素1推入到arr数组对象中。

* 向集合bar中推入不存在的数组对象和值。原记录中不存在数组对象name，如有记录：

 ```lang-json
 { arr: [1,2], age: 20 }
 ```

 ```lang-javascript
 > db.foo.bar.update({ $push: { name: "Tom" } }, { name: { $exists: 0 } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [1,2], age: 20, name: ["Tom"] }
 ```

 原记录中不存在数组对象name，使用$push操作符，会将name以数组对象的形式推入到记录中。
 
* 将上述集合bar中age字段的值推入到arr字段中
 
 ```lang-javascript
 > db.foo.bar.update({ $push: { arr: { $field: "age" } } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [ 1, 2, 20 ], age: 20, name: [ "Tom" ] }
 ```
