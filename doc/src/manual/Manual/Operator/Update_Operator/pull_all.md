
##语法##

```lang-json
{ $pull_all: { <字段名1>: [ <值1>, <值2>, ..., <值N> ], <字段名2>: [ <值1>, <值2>, ..., <值N> ], ... } }
```

##描述##

$pull_all 与 [$pull][pull] 功能类似。

区别在于：

$pull 只能匹配某个字段的一个值，$pull_all 能匹配某个字段的多个值。

执行一次 $pull_all

```lang-json
{ $pull_all: { <字段名1>: [ <值1>, <值2>, ..., <值N> ] } }
```

相当于执行多次 $pull

```lang-json
{ $pull: { <字段名1>: <值1> } }
{ $pull: { <字段名1>: <值2> } }
...
{ $pull: { <字段名1>: <值N> } }
```

##示例##

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { arr: [ 1, 2, 4, 5 ], age: 10, name: [ "Tom", "Mike" ] }
 ```

 删除 arr 字段中为 2 或者为 3 的元素，删除 name 字段中为"Tom"的元素

 ```lang-javascript
 > db.sample.employee.update( { $pull_all: { arr: [ 2, 3 ], name: [ "Tom" ] } } )
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [ 1, 4, 5 ], age: 10, name: [ "Mike" ] }
 ```

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { arr: [ 1, 3, 4, 5 ], age: 10, name: [ "Tom", "Mike" ] }
 ```

 删除 arr 字段中为 4 或者 为 5 的元素

 ```lang-javascript
 > db.sample.employee.update( { $pull_all: { arr: [ 4, 5 ] } } )
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [ 1, 3 ], age: 10, name: [ "Tom", "Mike" ] }
 ```


[^_^]:
    本文使用的所有引用及链接
[pull]:manual/Manual/Operator/Update_Operator/pull.md