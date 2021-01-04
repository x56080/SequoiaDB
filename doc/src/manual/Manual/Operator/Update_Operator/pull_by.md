
##语法##

```lang-json
{ $pull_by: { <字段名1>: <值1>, <字段名2>: <值2>, ... } }
```

##描述##

$pull_by 与 [$pull][pull] 功能类似。

区别在于：

$pull_by 若<值>为对象时，只要<值>中的所有字段值与数组元素的字段值相同，则认为匹配成功，并删除数组中的元素。

$pull 若<值>为对象时，需要数组元素的每个字段值都与<值>中的字段值相同，才认为匹配成功，并删除数组中的元素。

##示例##

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { a: [ 1, 2, 3, 2 ], b: [ 4, 5, 6 ] }
 ```
 
 删除 a 字段中为 2 的元素，删除 b 字段中为 5 的元素

 ```lang-javascript
 > db.sample.employee.update( { $pull_by: { a: 2, b: 5 } } )
 ```

 此操作后，记录更新为：

 ```lang-json
 { a: [ 1, 3 ], b: [ 4, 6 ] }
 ```

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { a: [ { id: 1, num: 1 }, { id: 2, num: 2 }, { id: 3, num: 3 }, { id: 4, num: 4 }, { id: 2,  num: 3 } ] }
 ```

 删除 a 字段中含有 { id: 2 } 的元素

 ```lang-javascript
 > db.sample.employee.update( { $pull_by: { a: { id: 2 } } } )
 ```

 此操作后，记录更新为：

 ```lang-json
 { a: [ { id:1, num: 1 }, { id: 3, num: 3 }, { id: 4, num: 4 } }
 ```

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { a: [ { id: 1, num: 1 }, { id: 2, num: 2 }, { id: 3, num: 3 }, { id: 4, num: 4 }, { id: 2,  num: 3 } ] }
 ```

 删除 a 字段中含有 { id: 1, num: 3 } 的元素

 ```lang-javascript
 > db.sample.employee.update( { $pull_by: { a: { id: 1, num: 3 } } } )
 ```

 此操作后，没有命中任何元素，记录依然为：

 ```lang-json
 { a: [ { id: 1, num: 1 }, { id: 2, num: 2 }, { id: 3, num: 3 }, { id: 4, num: 4 }, { id: 2,  num: 3 } ] }
 ```

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { a: [ { id: 1, num: 1 }, { id: 2, num: 2 }, { id: 3, num: 3 }, { id: 4, num: 4 }, { id: 2,  num: 3 } ] }
 ```

 删除 a 字段中含有 { id: 2, num: 3 } 的元素

 ```lang-javascript
 > db.sample.employee.update( { $pull_by: { a: { id: 2, num: 3 } } } )
 ```

 此操作后，记录更新为：

 ```lang-json
 { a: [ { id: 1, num: 1 }, { id: 2, num: 2 }, { id: 3, num: 3 }, { id: 4, num: 4 } ] }
 ```


[^_^]:
    本文使用的所有引用及链接
[pull]:manual/Manual/Operator/Update_Operator/pull.md