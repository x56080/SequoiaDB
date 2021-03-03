##语法##

```
{ $set: { <字段名1>: <值1>, <字段名2>: <值2>, ... } }
```

##描述##

$set操作是将指定的“<字段名>”更新为指定的“<值>”。如果原记录中没有指定的字段名，那将字段名和值填充到记录中；如果原记录中存在指定的字段名，那么将字段名的值更新为指定的值。

##示例##

* 选择集合bar下不存在age字段的记录，使用$set更新这些记录

 ```lang-javascript
 > db.foo.bar.update({ $set: { age: 5, ID: 10 } }, { age: { $exists: 0 } })
 ```

* 更新集合bar下的所有记录，使所有记录的字段str的值更新为“abc”

 ```lang-javascript
 > db.foo.bar.update({ $set: { str: "abd" } })
 ```

* 使用$set更新嵌套数组对象里面的元素。字段名arr在集合bar中是一个嵌套数组对象，例如有两条记录：{arr:[1,2,3],name:"Tom"},{name:"Mike",age:20}第二条记录没有arr字段名

 ```lang-javascript
 > db.foo.bar.update({ $set: { "arr.1": 4 } }, { name: { $exists: 1 } })
 ```

 此操作是选择含有name字段的所有记录，然后使用$set更新这些记录的数组对象arr。如果原记录中没有数组对象arr，使用$set会将arr字段以嵌套对象的方式插入到记录中。上面两条记录更新之后为：

 ```
 { arr: [1,4,3], name: "Tom" }, { arr: { "1": 4 }, name: "Mike", age: 20 }
 ```
