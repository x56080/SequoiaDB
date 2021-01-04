
##语法##

```lang-json
{ $pull_all_by: { <字段名1>: [ <值1>,<值2>,...,<值N> ], <字段名2>: [ <值1>,<值2>,...,<值N> ], ... } }
```

##描述##

$pull_all_by 与 [$pull_all][pull_all] 功能类似。

区别在于：

$pull_all_by 若<值>为对象时，只要<值>中的所有字段值与数组元素的字段值相同，则认为匹配成功，并删除数组中的元素。

$pull_all 若<值>为对象时，需要数组元素的每个字段值都与<值>中的字段值相同，才认为匹配成功，并删除数组中的元素。

##示例##

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { a: [ 1, 2, 3, 2 ], b: [ 4, 5, 6 ] }
 ```

 删除 a 字段中为 1 或者为 2 的元素，删除 b 字段中为 5 的元素

 ```lang-javascript
 > db.sample.employee.update( { $pull_all_by: { a: [ 1, 2 ], b: [ 5 ] } } )
 ```

 此操作后，记录更新为：

 ```lang-json
 { a: [ 3 ], b: [ 4, 6 ] }
 ```

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { a: [ { id: 1, num: 1 }, { id: 2, num: 2 }, { id: 3, num: 3 }, { id: 4, num: 4 }, { id: 2,  num: 3 } ] }
 ```

 删除 a 字段中含有 { id: 1 } 或者 { id: 2 } 的元素

 ```lang-javascript
 > db.sample.employee.update( { $pull_all_by: { a : [ { id: 1 }, { id: 2 } ] } } )
 ```

 此操作后，记录更新为：

 ```lang-json
 { a: [ { id: 3, num: 3 }, { id: 4, num: 4 } }
 ```


[^_^]:
    本文使用的所有引用及链接
[pull_all]:manual/Manual/Operator/Update_Operator/pull_all.md