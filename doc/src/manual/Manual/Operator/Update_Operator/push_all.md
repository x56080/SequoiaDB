
##语法##

```lang-json
{ $push_all: {<字段名1>:[<值1>,<值2>,...,<值N>], <字段名2>: [<值1>,<值2>,...,<值N>], ... } }
```

##描述##

$push_all向指定数组对象<字段名1>推入每一个指定值[<值1>,<值2>,...,<值N>]。操作对象必须为数组类型的字段。如果记录中不存在指定的数组对象，向记录推入指定的数组对象和每一个指定的值[<值1>,<值2>,...,<值N>]；如果指定的值存在数组对象中，同样被推入到数组对象中。

##示例##

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { arr: [1,2,4,5], age: 10, name: ["Tom","Mike"] }
 ```

 向集合中的 arr 数组对象推入[1,2,8,9]数组

 ```lang-javascript
 > db.sample.employee.update({ $push_all: { arr: [1,2,8,9] } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [1,2,4,5,1,2,8,9], age: 10, name: ["Mike"] }
 ```

  > **Note:**
  >
  > 虽然原来记录 arr 对象有元素 1 和 2，使用 $push_all 操作符，会将[1,2,8,9]全部值推入到数组对象 arr 中。

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { arr: [1,3,4,5], age: 10 }
 ```

 向集合中推入不存在的数组对象 name

 ```lang-javascript
 > db.sample.employee.update({ $push_all: { name: ["Tom","Jhon"] } }, { name: { $exists: 0 } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [1,3,4,5], age: 10, name: ["Tom","Mike"] }
 ```
